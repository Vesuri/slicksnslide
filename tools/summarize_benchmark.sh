#!/bin/sh
# Offline statistics: no debugger stops or analysis inside measured updates.
set -eu
for log in "$@"; do
  awk '
    /^BENCHMARK frames=/ { split($2,f,"="); expected=f[2]+0; print FILENAME; print }
    /^WORK_SUM=/ { split($1,s,"="); expected_sum=s[2]+0 }
    /^PROFILE_DETAIL=/ { print }
    /^WORK_SAMPLE / {
      split($3,w,"="); split($4,p,"="); v[++n]=w[2]+0; sum+=v[n];
      b=int(p[2]/32); counts[b]++; sums[b]+=v[n];
      if(v[n]>312) over[b]++;
      if(v[n]>maxima[b]) maxima[b]=v[n];
    }
    END {
      if(!n || n!=expected || sum!=expected_sum) {
        print "ERROR: incomplete/inconsistent timing samples"; exit 1
      }
      for(i=2;i<=n;i++) {
        x=v[i]; j=i-1;
        while(j>0 && v[j]>x) { v[j+1]=v[j]; j-- }
        v[j+1]=x;
      }
      # Nearest-rank percentiles; PAL raster line ~= 20/312 ms.
      printf "work_ms mean=%.3f p50=%.3f p90=%.3f p95=%.3f p99=%.3f max=%.3f n=%d\n",sum/n*20/312,v[int((n+1)/2)]*20/312,v[int((90*n+99)/100)]*20/312,v[int((95*n+99)/100)]*20/312,v[int((99*n+99)/100)]*20/312,v[n]*20/312,n;
      for(b=0;b<=8;b++) if(counts[b])
        printf "particles=%d..%d n=%d mean_ms=%.3f max_ms=%.3f over20=%d\n",32*b,32*b+31,counts[b],sums[b]/counts[b]*20/312,maxima[b]*20/312,over[b];
    }
  ' "$log"
done
