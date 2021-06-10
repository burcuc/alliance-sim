# alliance-sim

The simulation of the network communication of alliance protocols are implemented in ns3-3.29/scratch:
  * bcast.cc - used for one-to-all protocol
  * tree.cc - used for k-ary tree and two-level protocols
  * hyper.cc - used for MHAP
 
BRITE configuration files are in BRITE/conf_files:
  * TDBWx.conf where x is the number of autonomous systems. The number of processors (N) is x*128

To compile:
- in BRITE:
  - run "make"
- in ns-3.29:
  - run "./waf configure --with-brite=../BRITE" 
  - run "./waf"
  
To run:
- in ns-3.29:
- run "./waf --run scratch/*EXPERIMENT_TYPE* --N=*SIZE* --no_runs=*NO_RUNS* --topology=*TOPOLOGY*  *ADDITIONAL_ARGS*"
  - *EXPERIMENT_TYPE*: can be "bcast", "hyper" or "tree". corresponds to the simulation files in ns-3/ns-3.29/scratch
  - *N*: number of processors
  - *NO_RUNS*: number of times the protocol is run, only the last run will be printed. earlier runs will suffer from TCP slow start
  - *TOPOLOGY*:  can be "star", "star_as" or "brite". ("star_as"=star of stars)
  - *ADDITIONAL_ARGS*: additional arguments that you may use for different types of experiments
    - for hypercube simulation:
      - --C=*C*, sets the compaction factor, usage: --C=2, --C=4
      - --group, uses grouping with the hypercube, usage: --group
    - for tree simulation: 
      - --B=*B*, sets the branching factor, usage: --B=4, --B=1024
      - --group, uses grouping with the tree, usage: --group
      - --bcast, turns into the 2 level protocol, usage: --bcast

Example run commands:
- ./waf --run scratch/bcast --N=256 --no_runs=30 --topology=star
- ./waf --run scratch/tree --N=1024 --no_runs=30 --topology=brite --B=4
- ./waf --run scratch/tree --N=4096 --no_runs=30 --topology=star_as --bcast (for 2 level protocol)
- ./waf --run scratch/hyper --N=512 --no_runs=30 --topology=brite --C=4 --group



