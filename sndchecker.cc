#include <stdio.h>      /* printf */
#include <stdlib.h>
#include <cmath>        // std::abs
#include <vector> 
#include <numeric>
#include <sndfile.hh>

/* 8K chunks vs 1K chunks = about a 4x speedup */

#define		BUFFER_LEN		8192

float threshold = 0.13;   // Anything above this loudness threshold, we consider to be "good"

/* 22000 samples = 500 mS @ 44.KHz */
int bucket_size = 22000; // number of samples per bucket, should really be derived from file... 

/* track goes here */
std::vector<float> samples;
std::vector<float> rms_buckets;

static void read_file (const char * fname) 
{	 
  SndfileHandle mySndfileHandle; 
  mySndfileHandle = SndfileHandle(fname); 

  int channels = mySndfileHandle.channels(); 
  int frames = mySndfileHandle.frames(); 
  printf(" thresh: %f\nbkt_siz: %d\n" , threshold, bucket_size);
  printf("   chan: %d\n frames: %d\n" , channels, frames);

  // We'll load in BUFFER_LEN samples at a time, and stuff them into a buffer 
  // We need to ensure that the buffer is long enough to hold all of the 
  // Channel data  */
  uint bufferSize = BUFFER_LEN * channels; 
  
  // initialize our read buffer
  float readbuffer[bufferSize]; 
  int readcount; 
  int i;
  int j; 
  int readpointer = 0;
  
  // converts all multichannel files to mono by averaging the channels
  // This is probably not an optimal way to convert to mono
  float monoAverage;
  
  while ((readcount = mySndfileHandle.readf(readbuffer, 1024))) {
    readpointer = 0;
    for (i = 0; i < readcount; i++) {
      // for each frame...
      monoAverage = 0;
      for(j = 0; j < channels; j++) {
	monoAverage += readbuffer[readpointer + j];
      }
      monoAverage /= channels;
      readpointer += channels;
      // add the averaged sample to our vector of samples
      samples.push_back(monoAverage);
    }
  }

} /* read_file */

void analyze_file() {
  /* attempt to calcuate a "quality" metric for the track. 
   * 
   * my quick and dirty calculation, aka the working algorithm is:
   *
   *   divide up the track into a number of buckets, around 500mS / bucket
   *   for each bucket, calculate rms for the entire bucket
   *     at the end of the bucket, calculate the rms / "loudness" of the bucket
   *     if loudness > threshold
   *        count bucket as good
   *     reset the per-bucket counter
   *   divide the total number of buckets by the successful buckets.
   *   return this as a score between 0-100%.
   *
   * Alternate Ideas instead of "loudness thresholds": 
   *  - Use Standard deviation - tried it, not goot enough.
   *  - IQR, kurtosis, entropy, 95% range
   */

  int bucket_pos;
  int buckets_good = 0;
  float this_bucket_total = 0;

  unsigned long buckets;

  printf("samples: %ld\n", samples.size());

  bucket_pos = 0;
  buckets = samples.size() / bucket_size;
  printf("buckets: %ld\n", buckets);
  
  // for all samples
  for (int i=0; i < samples.size(); i++) { 

    this_bucket_total += std::abs(samples[i]) * std::abs(samples[i]);
    bucket_pos++;

    if (bucket_pos > bucket_size) {
      float rms = sqrt((this_bucket_total / bucket_size)); 

      if (rms > threshold) { 
	buckets_good++;
      }

      /* stash away the bucket data for further analysis */
      rms_buckets.push_back(rms);

      /* reset */
      bucket_pos = 0;
      this_bucket_total = 0;

    }
  }
  printf("   good: %i\npctgood: %.02f %%\n" , buckets_good, ( (float)buckets_good/buckets ) * 100);

};

int main (int argc,char * argv[])
{

  if ((argc < 2) || (argc > 4)) { 
    printf("\nUsage: %s soundfile [threshold] [samples_per_bucket]\n", argv[0]);
    printf("   threshold          RMS threshold for a good bucket (default %0.3f)\n", threshold);
    printf("   samples_per_bucket Samples per bucket (default %d)\n\n", bucket_size);
    exit(1);
  }

  if (argc >= 3) { 
    std::string str (argv[2]);
    threshold = std::stof(str);
  }

  if (argc == 4) { 
    std::string str (argv[3]);
    bucket_size = std::stoi(str);
  }

  read_file (argv[1]) ;
  analyze_file();

  /* Get min, max, stddev */
  float maxrms = 0;
  float minrms = 99999;
  float avgrms = 0;
  float totrms = 0;
  float sum = std::accumulate(rms_buckets.begin(), rms_buckets.end(), 0.0);

  for (int i=0; i < rms_buckets.size(); i++) { 
      if (rms_buckets[i] > maxrms) { maxrms = rms_buckets[i]; };
      if (rms_buckets[i] < minrms) { minrms = rms_buckets[i]; };
  }
  avgrms = sum / rms_buckets.size();

  printf(" minrms: %f\n maxrms: %f\n avgrms: %f\n", minrms, maxrms, avgrms);

  /* let's try this the c++ way... */

  float mean = sum / rms_buckets.size();
  float sq_sum = std::inner_product(rms_buckets.begin(), rms_buckets.end(), rms_buckets.begin(), 0.0);
  float stddev = std::sqrt(sq_sum / rms_buckets.size() - mean * mean);

  printf("   mean: %f\n stddev: %f\n", mean, stddev);
  printf(" minrms: %f\n maxrms: %f\n avgrms: %f\n", minrms, maxrms, avgrms);

  /* variance */
  float variance;
  for (int i = 0; i < rms_buckets.size(); i++)
    {
      variance += (rms_buckets[i] - mean)*(rms_buckets[i] - mean);
    }
  variance = (double)(variance)/(rms_buckets.size() - 1);
  printf("variance: %f\n", variance);

  /* skewnewss */
  float S = (float)sqrt(variance);
  float skewness;

  for (int i = 0; i < rms_buckets.size(); i++)
    skewness += (rms_buckets[i] - mean)*(rms_buckets[i] - mean)*(rms_buckets[i] - mean);
  skewness = skewness/(rms_buckets.size() * S * S * S);
  printf("skewness: %f\n", skewness);

  /* kurtosis */
  float k;
  for (int i = 0; i < rms_buckets.size(); i++)
    k += (rms_buckets[i] - mean)*(rms_buckets[i] - mean)*(rms_buckets[i] - mean)*(rms_buckets[i] - mean);
  k = k/(rms_buckets.size()*S*S*S*S);
  k -= 3; 
  printf("kurtosis: %f\n", k);
  
  /* dump the SVG bucket list */
  printf("\nRMS Buckets\n\n");
  printf("http://sparksvg.me/bar.svg?");
  for (int i=0; i < rms_buckets.size(); i++) { 
    printf("%d", int(rms_buckets[i]*1000));
    if (i+1 < rms_buckets.size()) { printf(","); }
  }


  puts ("\n\nDone.\n") ;
  return 0 ;
} /* main */




