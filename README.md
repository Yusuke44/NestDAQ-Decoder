# NestDAQ-Decoder

Decoders for NestDAQ data and AMANEQ scaler data.

`decode_tfs` reads FileSink output from the following chain and writes a ROOT TTree with one entry per trigger slice:

```
Sampler → STFBuilder → TimeFrameBuilder → LogicFilter → TimeFrameSlicerByLogicTiming → FileSink
```

## Requirements

- Linux or macOS (the headers use `<sys/time.h>`)
- A C++17 compiler
- [ROOT](https://root.cern/) 6 (`root-config` must be in your `PATH`)
- [nestdaq-user-impl](https://github.com/spadi-alliance/nestdaq-user-impl), for the data format headers

## Setup

Clone this repository, then clone nestdaq-user-impl into it:

```bash
git clone https://github.com/Yusuke44/NestDAQ-Decorder.git
cd NestDAQ-Decorder
git clone https://github.com/spadi-alliance/nestdaq-user-impl
```

The headers this decoder uses from nestdaq-user-impl:

| Header | Source |
|---|---|
| `FileSinkHeader.h`, `FileSinkTrailer.h` | nestdaq-user-impl |
| `TimeFrameHeader.h`, `SubTimeFrameHeader.h` | nestdaq-user-impl |
| `HeartbeatFrameHeader.h`, `FilterHeader.h` | nestdaq-user-impl |
| `AmQStrTdcData_RM.h` | `include/` in this repository (`AmQStrTdcData.h` with streaming RM support added) |

## Build

```bash
g++ -std=c++17 -O2 -Iinclude -Inestdaq-user-impl decoder/decode_tfs.cc $(root-config --cflags --libs) -o decode_tfs
g++ -std=c++17 -O2 -Iinclude -Inestdaq-user-impl decoder/decode_tf.cc  $(root-config --cflags --libs) -o decode_tf
```

## Usage

```bash
./decode_tfs input.dat output.root
```

Sample data is in `sample-data/NestDAQ/`:

| File | Chain |
|---|---|
| `TFS.dat` | … → LogicFilter → TimeFrameSlicer → FileSink (use `decode_tfs`) |
| `TFB-STFB.dat` | Sampler → STFBuilder → TimeFrameBuilder → FileSink (use `decode_tf`) |

## Output tree (`decode_tfs`)

One entry per SLICE time frame (one trigger).

| Branch | Type | Description |
|---|---|---|
| `trg_time` | `uint32` | Trigger time in tdc4n units (= LR-TDC `tdc >> 2`). Multiply by 4 to compare with `tdc`. |
| `n_triggers` | `uint32` | Number of triggers in this heartbeat frame |
| `flt_type`, `worker_id`, `elapse_time`, `flt_sec`, `flt_usec`, `tth_type` | | LogicFilter header fields |
| `tdc`, `tot`, `ch`, `data_type` | `vector` | Hits inside the trigger window |
| `fem_type`, `ip_addr`, `ip2`, `ip3` | `vector` | FEM of each hit (`ip3` = last octet of the IP address) |
| `frame_number`, `laccp_offset`, `hb_flags` | `vector` | Heartbeat delimiter that follows each hit |

## License

The data format headers come from [nestdaq-user-impl](https://github.com/spadi-alliance/nestdaq-user-impl) (MIT License).
