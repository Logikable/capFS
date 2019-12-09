# CapFS Benchmarking
Details: https://docs.google.com/document/d/1UFXS5OLRtr4HG6aCcXR9TU7djOrmNMDJsQIit0jIE9w/edit?fbclid=IwAR3hotN1X4J0LvWQ0_kne9FbeuT9jeWlHlsp6cwnEP_nym0Ad6ev5whrvTU
Check the Evaluation section.

## Environment
Ubuntu 18 EC2 VM. 
```
Spec
Intel(R) Xeon(R) CPU E5-2686 v4 @ 2.30GHz
2 cores
8G memory
```

## FS
- EFS: Amazon's NFS 
- CapFS: GDP append-only logging filesystem

## EFS Setup
EFS is set and mounted on the directory efs_mount_point
CapFS will be mounted at the dir cap-fs_mount_point

## Possible Performance Gap
Presumably, CapFS will have much worse performance because of host environment. 
We need to figure out how to take into account of GDP's poor hosting env.

## MicroBenchmarking
```
create
read
write
```

```
python main.py --help # will print useful options
python main.py -m micro -c [capFs-mount-directory] -e [efs-mount-directory]
```
This command line tool will perform micro benchmark of read/create/write on given mounted directories.

## MacroBenchmarking
```
# sequential read/write
# random read/write
# cp 
# untarring
```

## Metrics
- Latencies
- Read/Write throughput

