#include <stdio.h>      /* printf */
#include <stdlib.h>
#include <cmath>        // std::abs
#include <vector> 
#include <numeric>
#include <sndfile.hh>
#include <getopt.h>
#include <sys/stat.h>
#include <string.h>

/* 8K chunks vs 1K chunks = about a 4x speedup */

#define		BUFFER_LEN		8192

float threshold = 0.13;   // Anything above this loudness threshold, we consider to be "good"

/* 22000 samples = 500 mS @ 44.KHz */
int bucket_size = 22000;  // number of samples per bucket, should really be derived from file... 

/* track goes here */
std::vector<float> samples;
std::vector<float> rms_buckets;

/* options */
int opt_v = 0;
int opt_j = 0;
int opt_h = 0;

inline bool file_exists (const char name[]) { 
  struct stat buffer;   
  return (stat (name, &buffer) == 0); 
}

static bool read_file (const char * fname) 
{	 
  SndfileHandle mySndfileHandle;

  if (!file_exists(fname)) {
    return false;
  }

  mySndfileHandle = SndfileHandle(fname);

  int channels = mySndfileHandle.channels(); 
  int frames = mySndfileHandle.frames(); 

  if (opt_v) { 
    printf(" thresh: %f\nbkt_siz: %d\n" , threshold, bucket_size);
    printf("   chan: %d\n frames: %d\n" , channels, frames);
  }

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

  float this_bucket_total = 0;

  unsigned long buckets;
  unsigned long buckets_good = 0;
  unsigned long bucket_pos;

  bucket_pos = 0;
  buckets = samples.size() / bucket_size;

  if (opt_v) { 
    printf("samples: %ld\n", samples.size());
    printf("buckets: %ld\n", buckets);
  }
  
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

  if (opt_j) { 
    printf("{ \"buckets\":\"%lu\", \"good\":\"%lu\", \"pctgood\":\"%.02f\" }\n" , buckets,  buckets_good, ( (float)buckets_good/buckets ) * 100);
  } else { 
    printf("   good: %lu\npctgood: %.02f %%\n" , buckets_good, ( (float)buckets_good/buckets ) * 100);
  }

};

void usage(char *pgmname) { 
  printf("Usage: %s [options] filename\n",pgmname);
  printf("\n");
  printf("sndchecker returns a score indicating how many buckets in a file\n");
  printf("conform to a specified loudness threshold.\n");
  printf("\n");
  printf("  --threshold  power limit for a \"good\" bucket 0.0 to 1.0 [defaut: 0.13]\n");
  printf("  --size       samples per bucket [defaut: 22000 / 500mS at 44.1Khz]\n");
  printf("  --json       return score in json\n");
  printf("  --histogram  return a URL with the bucket list for graphing\n");
  printf("  --verbose    verbose logging and statistics\n");
  printf("\n");
  exit(1);
}

int main (int argc, char **argv)
{

  int c;

  while (1) {
    std::string str;
    int this_option_optind = optind ? optind : 1;
    int option_index = 0;
    
    static struct option long_options[] = {
      {"threshold",  required_argument, 0, 't' },
      {"size",       required_argument, 0, 's' },
      {"verbose",    no_argument,       0, 'v' },
      {"histogram",  no_argument,       0, 'h' },
      {"json",       no_argument,       0, 'j' },
      {"file",       required_argument, 0,  0 },
      {0,         0,                 0,  0 }
    };

    c = getopt_long(argc, argv, "vhjt:s:f:",
                    long_options, &option_index);
    if (c == -1)
      break;

    switch (c) {
    case 'v':
      opt_v = 1;
      break;

    case 'h':
      opt_h = 1;
      break;

    case 'j':
      opt_j = 1;
      break;

    case 't':
      str.assign(optarg, strlen(optarg));
      threshold = std::stof(str);
      break;

    case 's':
      str.assign(optarg, strlen(optarg));
      bucket_size = std::stoi(str);
      break;

    default:
      printf("Invalid argument -%c.\n", c);
      usage(argv[0]);
      break;
    }
  }

  /* done with opts */
  if (argv[optind] == NULL) { 
    printf("filename missing.\n");
    usage(argv[0]);
  }

  if (! read_file(argv[optind]) ) { 
    printf("File %s does not exist.\n", argv[optind]);
    exit(127);
  }

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


  /* let's try this the c++ way... */

  float mean = sum / rms_buckets.size();
  float sq_sum = std::inner_product(rms_buckets.begin(), rms_buckets.end(), rms_buckets.begin(), 0.0);
  float stddev = std::sqrt(sq_sum / rms_buckets.size() - mean * mean);

  /* variance */
  float variance;
  for (int i = 0; i < rms_buckets.size(); i++)
    {
      variance += (rms_buckets[i] - mean)*(rms_buckets[i] - mean);
    }
  variance = (double)(variance)/(rms_buckets.size() - 1);

  /* skewnewss */
  float S = (float)sqrt(variance);
  float skewness;

  for (int i = 0; i < rms_buckets.size(); i++)
    skewness += (rms_buckets[i] - mean)*(rms_buckets[i] - mean)*(rms_buckets[i] - mean);
  skewness = skewness/(rms_buckets.size() * S * S * S);
  
  /* kurtosis */
  float k;
  for (int i = 0; i < rms_buckets.size(); i++)
    k += (rms_buckets[i] - mean)*(rms_buckets[i] - mean)*(rms_buckets[i] - mean)*(rms_buckets[i] - mean);
  k = k/(rms_buckets.size()*S*S*S*S);
  k -= 3; 
  
  if (opt_v) { 
    printf("   mean: %f\n stddev: %f\n", mean, stddev);
    printf(" minrms: %f\n maxrms: %f\n avgrms: %f\n", minrms, maxrms, avgrms);
    printf("variance: %f\n", variance);
    printf("skewness: %f\n", skewness);
    printf("kurtosis: %f\n", k);
  }

  /* dump the SVG bucket list */
  if (opt_h) { 
    printf("\nRMS Buckets\n\n");
    printf("http://sparksvg.me/bar.svg?");
    for (int i=0; i < rms_buckets.size(); i++) { 
      printf("%d", int(rms_buckets[i]*1000));
      if (i+1 < rms_buckets.size()) { printf(","); }
    }
  }

  return 0 ;
} /* main */




