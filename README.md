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

## Parsing

### Date format

There are two parsers in the program: the command line parser, and the file
parser. Both use the same parser for dates.

Dates are parsed into a `date_t` structure that stores it as various components
(year, month, day, ...) all packed on 64bit, and ordered in a way that guarantee
that dates are comparable as 64bit integers. This allows the range to be easily
expressed as non-fully qualified dates.

For example, if we provide the date '2015-08-01' on command line, it is stored
in a integer in the form:

```
 2015 08 01 XX XX XX
 YYYY MM DD hh mm ss
```

In case we want a lower-bound, we fill the `XX` with `0`s, in case we want an
upper bound, we fill the `XX` with `F`s. In the end, we do have a strictly
ordered format for date that support partial-definition.

### Decimation on date

The decimation on date is performed directly during the parsing of the input
file.

Because dates are stored as integers, date comparison is quick. As a consequence,
we can perform the decimation of log lines that are not in the expected range
of date directly while parsing the file. The ensures we won't have to perform
heavy computation on lines that we don't care about.


## Distinct queries

The implementation of the `distinct` command is done using an `unordered_set`.
This data-structure provides the key-deduplication and provide the interface
to efficiently get the count.

Moreover, as opposed to the simple `set`, it is unordered and thus free the CPU
from some extra computation.

The insertion in an `unordered_set`, which is implemented using a hash-table,
has a complexity of amortized O(1). This means that, if we have few collisions
(keys with the same hash value) the insertion will be done in constant time.

The `unordered_set` must store the keys, so it contains one instance of every
single distinct search.

The workflow of the command is:

* parse the files (the total size of the files is named `S`) and decimate lines
  based on the date: this has a complexity of `O(N)` where `N` is the number of
  lines and produces `M` lines to process in the following step (`M <= N`).
* insert the search strings in the `unordered_map`. This has a complexity of
  `O(M)`, and will store K distinct keys (`K <= M`). The size of the storage
  is the sum of the lenght of the K keys (plus some overhead). So, the
  memory usage is `O(K * strlen(K))` with `K * strlen(K) < S`.
* count the number of entries in the `unordered_set`.

As a consequence, the parser has to read `S` bytes, and the complexity can go up
to `O(N)` insertions and `O(S)` bytes stored in memory.


## Top n queries

The implementation of the `top n` command is done using an `unordered_map` from
the search string to the number of occurrences of that string. When all the
input files have been processed, the map is enumerated and the top-n entries
are stored in small vector. That second step uses a bubble-insertion which is
written quite naively, supposing `n` is small. For big values of `n` a binary
search should be used.

The first step has a complexity of O(M) and stores `O(K * strlen(K))` bytes in
memory. The second step has a complexity of `O(K * n)` (`n` is from `top n`).


In order to test the implementation, the results where confronted to those of the
following command:

```
cat ~/Downloads/hn_logs.tsv | cut -f 2 | sort | uniq -c | sort -n | tail -n 5
```

## Development and Tools

The code is probably not idiomatic C++. It was written on macOS using VIM.
Memory checking was done using Address Sanitizer, and some profiling was
done using Instruments.

A minimalistic `Makefile` is provided.
