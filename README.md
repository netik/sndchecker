# sndchecker
Experimental code to see if it's possible to determine if a sound file is of "low quality" or "poor dynamic range."

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

We consider anything < 30% to be a "bad track" with serious fluctuations in loudness.

# False Positives
* Songs with many changes in loudness
* Ambient, "Quiet" tracks
* Classical
* Non-maximized / loud tracks 

Most Pop music works very well with this returning high scores. Old timey music, not so much.

# Todo
* Research better music identification techniques
