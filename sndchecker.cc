#include <stdio.h>      /* printf */
#include <stdlib.h>
#include <cmath>        // std::abs
#include <vector> 
#include <sndfile.hh>

/* 8K chunks vs 1K chunks = about a 4x speedup */

#define		BUFFER_LEN		8192

float threshold = 0.13;   // Anything above this loudness threshold, we consider to be "good"
int  bucket_size = 24000; // number of samples per bucket, should really be derived from file... 

/* track goes here */
std::vector<float> samples;   

static void read_file (const char * fname) 
{	 
  SndfileHandle mySndfileHandle; 
  mySndfileHandle = SndfileHandle(fname); 

  int channels = mySndfileHandle.channels(); 
  int frames = mySndfileHandle.frames(); 

  printf("%d channels, %d frames\n" , channels, frames);

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
   */

  int bucket_pos;
  int buckets_good = 0;
  float this_bucket_total = 0;

  unsigned long buckets;

  printf("samples: %ld\n", samples.size());

  /* 24000 samples = 500 mS @ 44.KHz */
  bucket_pos = 0;
  buckets = samples.size() / bucket_size;
  printf("buckets: %ld\n", buckets);
  
  // for all samples
  for (int i=0; i < samples.size(); i++) { 

    this_bucket_total += std::abs(samples[i]) * std::abs(samples[i]);
    bucket_pos++;

    if (bucket_pos > bucket_size) {
      float rms = sqrt((this_bucket_total / bucket_size)); 

      //      printf("rms: %f\n", rms);

      if (rms > threshold) { 
	buckets_good++;
      }

      /* reset */
      bucket_pos = 0;
      this_bucket_total = 0;

    }

  }
  printf("   good: %i\n    pct: %.02f %%\n" , buckets_good, ( (float)buckets_good/buckets ) * 100);

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

  puts ("Done.\n") ;
  return 0 ;
} /* main */




