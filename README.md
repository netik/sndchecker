# sndchecker
Experimental code to see if it's possible to determine if a sound file is of "low quality" or "poor dynamic range."

```
Usage: ./sndchecker [options] filename

sndchecker returns a score indicating how many buckets in a file
conform to a specified loudness threshold.

  --threshold  power limit for a "good" bucket 0.0 to 1.0 [defaut: 0.13]
  --size       samples per bucket [defaut: 22000 / 500mS at 44.1Khz]
  --json       return score in json
  --histogram  return a URL with the bucket list for graphing
  --verbose    verbose logging and statistics
```

# Algorithm
```
divide up the track into a number of buckets, around 500mS / bucket
for each bucket, calculate loudness for the entire bucket
  if loudness > threshold                                                                                     
     count bucket as good
     reset the per-bucket counter                                                                                

divide the total number of buckets by the "good" buckets.                                                 
  return this as a score between 0-100%
```

We consider anything < 40% to be a "bad track" with many quiet passages.

# False Positives
* Songs with many changes in loudness
* Ambient, "Quiet" tracks
* Classical
* Non-maximized / loud tracks 

Most Pop music works very well with this returning high scores. Old timey music, not so much.

# Todo
* Research better music identification techniques
