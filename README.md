# HN Query

This repository contains a small utility designed to query the query log
and extract some interesting indicators from those logs.

The utility supports two commands:

* Compute the number of distinct searches during a specific range of time:
```
query distinct --from "2015-08-01 08:30" --to "2015-08-01 10:00" path/to/log
```

* Extract the `n` most popular searches during a specific range of time:

```
query top 3 --from "2015-08-01 08:30" --to "2015-08-01 10:00" path/to/log
```
