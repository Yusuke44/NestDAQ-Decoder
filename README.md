# NestDAQ-Decoder

Decoders for NestDAQ (AMANEQ streaming TDC) data and HUL/AMANEQ scaler data.

> **Internal use only.** This repository is shared within the lab. Do not redistribute the code or the sample data outside the lab without permission.

## Contents

| Path | Description |
|---|---|
| `decoder/decode_tfs.cc` | Decoder for data taken with LogicFilter + TimeFrameSlicer (one tree entry per trigger) |
| `decoder/decode_tf.cc` | Decoder for data taken without the slicer (STFBuilder / TimeFrameBuilder output) |
| `scr/decode_scr.cc` | Decoder for scaler data read out by `read_scr` |
| `scr/read_scr.sh` | Script that reads scaler data periodically |
| `include/` | Data format headers (see [Third-party code](#third-party-code)) |
| `sample-data/NestDAQ/` | Sample data for testing |

## Requirements

- Linux or macOS (the headers use `<sys/time.h>`)
- A C++17 compiler
- [ROOT](https://root.cern/) 6 (`root-config` must be in your `PATH`)

## Build

```bash
git clone https://github.com/Yusuke44/NestDAQ-Decoder.git
cd NestDAQ-Decoder
g++ -std=c++17 -O2 -Iinclude decoder/decode_tfs.cc $(root-config --cflags --libs) -o decode_tfs
g++ -std=c++17 -O2 -Iinclude decoder/decode_tf.cc  $(root-config --cflags --libs) -o decode_tf
g++ -std=c++17 -O2 scr/decode_scr.cc -o decode_scr
```

## Usage

### NestDAQ data

```bash
./decode_tfs input.dat output.root
./decode_tf  input.dat output.root
```

Which decoder to use depends on the DAQ chain:

| DAQ chain | Decoder | Sample |
|---|---|---|
| Sampler → STFBuilder → TimeFrameBuilder → LogicFilter → TimeFrameSlicerByLogicTiming → FileSink | `decode_tfs` | `sample-data/NestDAQ/TFS.dat` |
| Sampler → STFBuilder → TimeFrameBuilder → FileSink | `decode_tf` | `sample-data/NestDAQ/TFB-STFB.dat` |

Try it with the sample data:

```bash
./decode_tfs sample-data/NestDAQ/TFS.dat tfs.root
```

### Scaler data

```bash
./decode_scr input.bin
```

`scr/read_scr.sh` calls `read_scr` from [hul-common-lib](https://github.com/spadi-alliance/hul-common-lib). Edit `EXEC_CMD`, the IP addresses and the output paths at the top of the script for your environment before running it.

## Output tree of `decode_tfs`

Tree name: `tree`. One entry per SLICE time frame (= one trigger).

| Branch | Type | Description |
|---|---|---|
| `trg_time` | `uint32` | Trigger time in **tdc4n units** (= LR-TDC `tdc >> 2`). Multiply by 4 to compare with `tdc`. |
| `n_triggers` | `uint32` | Number of triggers in this heartbeat frame |
| `flt_type`, `worker_id`, `elapse_time`, `flt_sec`, `flt_usec`, `tth_type` | scalar | LogicFilter header fields |
| `tdc`, `tot`, `ch`, `data_type` | `vector` | Hits inside the trigger window |
| `fem_type`, `ip_addr`, `ip2`, `ip3` | `vector` | FEM of each hit (`ip2`, `ip3` = 3rd and 4th octets of the IP address) |
| `frame_number`, `laccp_offset`, `hb_flags` | `vector` | Heartbeat delimiter following each hit |

All hit vectors in one entry have the same length.

## Known issues

`decode_tfs`:

- If the input file has no FileSink trailer (e.g. FileSink crashed), the last slice is lost.
- If the input file is truncated, the program may abort without writing the ROOT file.
- Trailing-edge hits (head `0x0D`) are not stored.
- Only LR-TDC (`femType = 6`) and HR-TDC (`femType = 5`) are decoded. Hits from other FEM types are skipped.

## Third-party code

The headers in `include/` are taken from [spadi-alliance/nestdaq-user-impl](https://github.com/spadi-alliance/nestdaq-user-impl) (commit `3dd6b69`), Copyright (c) 2023 spadi-alliance, distributed under the MIT License. See [`include/LICENSE.nestdaq-user-impl`](include/LICENSE.nestdaq-user-impl).

| File | Status |
|---|---|
| `FileSinkHeader.h`, `FileSinkTrailer.h`, `FilterHeader.h`, `HeartbeatFrameHeader.h`, `SubTimeFrameHeader.h`, `TimeFrameHeader.h` | Unmodified |
| `AmQStrTdcData_RM.h` | Modified from `AmQStrTdcData.h`: added streaming RM (`StreamingRM`, head `0x09`) support |

## Contact

Yusuke Nakayama (Tohoku University)
