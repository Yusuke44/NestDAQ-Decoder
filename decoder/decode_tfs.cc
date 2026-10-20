#include <cstdint>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

#include "TFile.h"
#include "TString.h"
#include "TTree.h"

#include "AmQStrTdcData_RM.h"
#include "FileSinkHeader.h"
#include "FileSinkTrailer.h"
#include "FilterHeader.h"
#include "HeartbeatFrameHeader.h"
#include "SubTimeFrameHeader.h"
#include "TimeFrameHeader.h"

using namespace std;

enum {
  kFileSinkHeader,
  kTimeFrame,
  kSubTimeFrame,
  kFilter,
  kHeartbeatFrame,
  kFileSinkTrailer,
  kNumHeaderType
};
enum {
  kAmQData,
  kAmQHeartbeat,
  kAmQHeartbeat2nd,
  kAmQTrailer,
  kAmQThrottlingT1Start,
  kAmQThrottlingT1End,
  kAmQThrottlingT2Start,
  kAmQThrottlingT2End,
  kAmQStreamingRM
};
enum {
  kHRTDC = 5,
  kLRTDC = 6,
  kNumFETypes = 2
};

constexpr uint64_t kMagicWords[kNumHeaderType] = {FileSinkHeader::v1::MAGIC,
  TimeFrame::v1::MAGIC,
  SubTimeFrame::v1::MAGIC,
  Filter::v1::MAGIC,
  HeartbeatFrame::MAGIC,
  FileSinkTrailer::v1::MAGIC};
constexpr int kDataSize[kNumHeaderType] = {38, 3, 6, 7, 2, 38};

bool read_u64_le(istream& is, uint64_t& out) {
  unsigned char b[8];
  if (!is.read(reinterpret_cast<char*>(b), 8)) return false;
  out = (uint64_t)b[0]
    | ((uint64_t)b[1] << 8)
    | ((uint64_t)b[2] << 16)
    | ((uint64_t)b[3] << 24)
    | ((uint64_t)b[4] << 32)
    | ((uint64_t)b[5] << 40)
    | ((uint64_t)b[6] << 48)
    | ((uint64_t)b[7] << 56);
  return true;
}

int CheckHeader(uint64_t buf) {
  for (int i = 0; i < kNumHeaderType; ++i) {
    if (buf == kMagicWords[i]) {
      return i;
    }
  }
  return -1;
}

void ReadExact(istream* ifs, void* dst, size_t n) {
  if (!ifs->read(reinterpret_cast<char*>(dst), (streamsize)n))
    throw runtime_error("EOF/IO error");
}

int CheckAMANEQHeader(AmQStrTdc::Data::v1::Bits bits) {
  namespace AmQ = AmQStrTdc::Data::v1;
  auto head = bits.head;
  if (head == AmQ::HeadTypes::Data) {
    return kAmQData;
  } else if (head == AmQ::HeadTypes::Heartbeat) {
    return kAmQHeartbeat;
  } else if (head == AmQ::HeadTypes::Heartbeat2nd) {
    return kAmQHeartbeat2nd;
  } else if (head == AmQ::HeadTypes::Trailer) {
    return kAmQTrailer;
  } else if (head == AmQ::HeadTypes::ThrottlingT1Start) {
    return kAmQThrottlingT1Start;
  } else if (head == AmQ::HeadTypes::ThrottlingT1End) {
    return kAmQThrottlingT1End;
  } else if (head == AmQ::HeadTypes::ThrottlingT2Start) {
    return kAmQThrottlingT2Start;
  } else if (head == AmQ::HeadTypes::ThrottlingT2End) {
    return kAmQThrottlingT2End;
  } else if (head == AmQ::HeadTypes::StreamingRM) {
    return kAmQStreamingRM;
  }
  return -1;
}

int main(int argc, char* argv[]) {
  if (argc != 3) {
    cerr << "./bin [input.dat] [output.root]" << endl;
    return -1;
  }
  auto in_name  = TString(argv[1]);
  auto out_name = TString(argv[2]);

  ifstream ifs(in_name);
  if (!ifs) {
    cerr << "failed to open " << in_name << endl;
    return -1;
  }

  auto out_file = TFile(out_name, "recreate");
  auto out_tree = TTree("tree", "tree");
  // filter
  uint16_t flt_type;
  uint32_t n_triggers;
  uint32_t worker_id;
  uint32_t elapse_time;
  uint64_t flt_sec;
  uint64_t flt_usec;
  uint16_t tth_type;
  uint32_t trg_time;
  //  vector<uint32_t> trg_type;
  // hit
  vector<uint32_t> fem_type;
  vector<uint32_t> ip_addr;
  vector<uint8_t>  ip2;
  vector<uint8_t>  ip3;
  vector<uint32_t> frame_number;
  vector<uint32_t> laccp_offset;
  vector<uint32_t> hb_flags;
  vector<uint64_t> tdc;
  vector<uint64_t> tot;
  vector<uint64_t> ch;
  vector<uint64_t> data_type;
  out_tree.Branch("flt_type",     &flt_type,    "flt_type/s");
  out_tree.Branch("n_triggers",   &n_triggers,  "n_triggers/i");
  out_tree.Branch("worker_id",    &worker_id,   "worker_id/i");
  out_tree.Branch("elapse_time",  &elapse_time, "elapse_time/i");
  out_tree.Branch("flt_sec",      &flt_sec,     "flt_sec/l");
  out_tree.Branch("flt_usec",     &flt_usec,    "flt_usec/l");
  out_tree.Branch("tth_type",     &tth_type,    "tth_type/s");
  out_tree.Branch("trg_time",     &trg_time,    "trg_time/i");
  //  out_tree.Branch("trg_type",     &trg_type);
  out_tree.Branch("fem_type",     &fem_type);
  out_tree.Branch("frame_number", &frame_number);
  out_tree.Branch("laccp_offset", &laccp_offset);
  out_tree.Branch("hb_flags",     &hb_flags);
  out_tree.Branch("ip_addr",      &ip_addr);
  out_tree.Branch("ip2",          &ip2);
  out_tree.Branch("ip3",          &ip3);
  out_tree.Branch("tdc",          &tdc);
  out_tree.Branch("tot",          &tot);
  out_tree.Branch("ch",           &ch);
  out_tree.Branch("data_type",    &data_type);

  auto clear_hits = [&]() {
    fem_type.clear();
    ip_addr.clear();
    ip2.clear();
    ip3.clear();
    frame_number.clear();
    laccp_offset.clear();
    hb_flags.clear();
    tdc.clear();
    tot.clear();
    ch.clear();
    data_type.clear();
  };


  bool in_meta  = false;
  bool in_slice = false;
  int  slice_counter = -1;
  vector<uint32_t> trg_list;
  //bool is_first = true;
  int      tmp_fem_type;
  uint32_t tmp_ip_addr;
  int      tmp_ip2;
  int      tmp_ip3;

  auto flush_slice = [&](){
    if (in_slice){
      out_tree.Fill();
    }
    in_slice = false;
    clear_hits();
  };

  
  uint64_t buf;
  //  int n_hbf = 0;
  bool have_tf_header = false;
  uint32_t tf_id_from_tf = 0;
  FileSinkHeader::v1::Header   h_fsh{};
  TimeFrame::v1::Header        h_tf{};
  SubTimeFrame::v1::Header     h_stf{};
  Filter::v1::Header           h_flt{};
  Filter::v1::TrgTimeHeader    h_tth{};
  Filter::v1::TrgTime          h_tt{};
  HeartbeatFrame::Header       h_hbf{};
  FileSinkTrailer::v1::Trailer h_fst{};
  AmQStrTdc::Data::v1::Bits    h_amq{};

  while (read_u64_le(ifs, buf)) {
    auto flag = CheckHeader(buf);
    //cerr << "data: " << hex << buf << endl;
    //cerr << "flag: " << dec << flag << endl;
    if (flag == -1) {
      cerr << "Invalid magic word: " << hex << buf << endl;
      break;
    }
    
    if (flag == kFileSinkHeader) {
      ReadExact(&ifs, reinterpret_cast<char*>(&h_fsh) + sizeof(uint64_t), sizeof(FileSinkHeader::v1::Header) - sizeof(uint64_t));
      cout << "FairMQ device type: " << h_fsh.fairMQDeviceType << endl;
      cout << "Run number: "         << h_fsh.runNumber        << endl;
      tm tm = *localtime(&h_fsh.startUnixtime);
      cout << "Start: " << put_time(&tm, "%Y-%m-%d %H:%M:%S %Z") << endl;
    }

    else if (flag == kTimeFrame) {
      flush_slice();
      ReadExact(&ifs, reinterpret_cast<char*>(&h_tf) + sizeof(uint64_t), sizeof(TimeFrame::v1::Header) - sizeof(uint64_t));
      tf_id_from_tf  = h_tf.timeFrameId;
      //      tf_id          = h_tf.timeFrameId;
      have_tf_header = true;
      //      n_hbf          = 0;
      //      hbf_id         = 0;
      //h_tf.Print();
      if((h_tf.type & TimeFrame::v1::META) != 0){
	in_meta       = true;
	slice_counter = -1;
	trg_list.clear();
      } else if ((h_tf.type & TimeFrame::v1::SLICE) != 0 ){
	in_meta  = false;
	in_slice = true;
	++slice_counter;
	
	if ((size_t)slice_counter < trg_list.size()) {
	  trg_time = trg_list[slice_counter];
	} else {
	  cerr << "More slices than triggers: TF=" << tf_id_from_tf
	       << " slice=" << slice_counter << " triggers=" << trg_list.size() << endl;
          trg_time = 0xFFFFFFFF;
	}
	//trg_time = (trg_tdc4n == kInvalid) ? kInvalid : trg_tdc4n * 4;

      }  else {
	// TF without slicer (or unknown type): not handled by this decoder
	in_meta = false;
	// if (n_unsliced_tf++ == 0) {
	//   cerr << "Warning: TF with type=" << h_tf.type
	//        << " (neither META nor SLICE) found; its hits are skipped." << endl;
	// }
      }
    }

    else if (flag == kSubTimeFrame) {
      ReadExact(&ifs, reinterpret_cast<char*>(&h_stf) + sizeof(uint64_t), sizeof(SubTimeFrame::v1::Header) - sizeof(uint64_t));
      if (have_tf_header && tf_id_from_tf != h_stf.timeFrameId) {
        cerr << "TimeFrame ID mismatch: TF=" << tf_id_from_tf << " STF=" << h_stf.timeFrameId << endl;
      }
      tmp_fem_type = h_stf.femType;
      tmp_ip_addr  = h_stf.femId;
      tmp_ip2      = ((tmp_ip_addr) >>  8) & 0xFF;
      tmp_ip3      = ((tmp_ip_addr) >>  0) & 0xFF;
    }

    else if (flag == kFilter) {
      if (!in_meta) {
	cerr << "Warning: Filter header outside META TF (TF=" << h_tf.timeFrameId << ")" << endl;
      }
      ReadExact(&ifs, reinterpret_cast<char*>(&h_flt) + sizeof(uint64_t), sizeof(Filter::Header) - sizeof(uint64_t));
      flt_type    = h_flt.type;
      n_triggers  = h_flt.numTrigs;
      worker_id   = h_flt.workerId;
      elapse_time = h_flt.elapseTime;
      flt_sec     = h_flt.timeSec;
      flt_usec    = h_flt.timeUSec;
      // trg_time.clear();
      // trg_time.resize(n_triggers);
      //      trg_type.resize(n_triggers);
      //cerr << Form("%d %d %d %d %d %d %d %d %d %d", h_flt.length, h_flt.hLength, h_flt.type, h_flt.timeFrameId, h_flt.numTrigs, h_flt.workerId, h_flt.numMessages, h_flt.elapseTime, h_flt.timeSec, h_flt.timeUSec) << endl;
      ReadExact(&ifs, reinterpret_cast<char*>(&h_tth), sizeof(Filter::TrgTimeHeader));
      tth_type = h_tth.type;
      uint32_t ntrg = (h_tth.length - h_tth.hLength)/8;
      if(n_triggers != ntrg){
	cerr << "Trigger Number Mismatch" << endl;
      }
      //cerr << Form("%d %d %d", h_tth.length, h_tth.hLength, h_tth.type) << endl;
      trg_list.resize(ntrg);
      for (int i = 0; i < ntrg; ++i) {
        read_u64_le(ifs, buf);
        memcpy(&h_tt, &buf, 8);
        trg_list[i] = h_tt.time;
	//        trg_type[i] = h_tt.type;
      }
      // fem_type.clear();
      // ip_addr.clear();
      // ip2.clear();
      // ip3.clear();
      // frame_number.clear();
      // laccp_offset.clear();
      // hb_flags.clear();
      // tdc.clear();
      // tot.clear();
      // ch.clear();
      // data_type.clear();
    }

    else if (flag == kHeartbeatFrame) {
      ReadExact(&ifs, reinterpret_cast<char*>(&h_hbf) + sizeof(uint64_t), sizeof(HeartbeatFrame::Header) - sizeof(uint64_t));
      //      hbf_id = n_hbf;
      //      ++n_hbf;
      auto length = h_hbf.length / 8 - kDataSize[kHeartbeatFrame];
      int nhits = 0;
      for (int i = 0; i < length; ++i) {
        read_u64_le(ifs, buf);
        memcpy(&h_amq, &buf, 8);
        auto amq_flag = CheckAMANEQHeader(h_amq);
	if (in_meta)continue;
	if (!in_slice)continue;
	
        if (amq_flag == kAmQData) {
          if (tmp_fem_type == kLRTDC) {
            tdc.push_back(h_amq.tdc);
            tot.push_back(h_amq.tot);
            ch.push_back(h_amq.ch);
            data_type.push_back(h_amq.dtype);
          } else if (tmp_fem_type == kHRTDC) {
            tdc.push_back(h_amq.hrtdc);
            tot.push_back(h_amq.hrtot);
            ch.push_back(h_amq.hrch);
            data_type.push_back(h_amq.hrdtype);
          } else {
	    cout << "Unknown FEM Type" << endl;
	    continue;
	  }
	  fem_type.push_back(tmp_fem_type);
	  ip_addr.push_back(tmp_ip_addr);
          ip2.push_back(tmp_ip2);
          ip3.push_back(tmp_ip3);
	  nhits++;
        } else if (amq_flag == kAmQHeartbeat) {
          frame_number.insert( frame_number.end(), nhits, h_amq.hbframe);
	  laccp_offset.insert( laccp_offset.end(), nhits, h_amq.toffset);
	  hb_flags.insert( hb_flags.end(), nhits, h_amq.hbflag);
	  nhits=0;
        } else if (amq_flag == kAmQHeartbeat2nd) {
        } else if (amq_flag == kAmQThrottlingT1Start) {
          cout << "Throttling T1 start" << endl;
          cout << "IP: " << int{tmp_ip3} << endl;
        } else if (amq_flag == kAmQThrottlingT1End) {
          cout << "Throttling T1 end"   << endl;
          cout << "IP: " << int{tmp_ip3} << endl;
        } else if (amq_flag == kAmQThrottlingT2Start) {
          cout << "Throttling T2 start" << endl;
          cout << "IP: " << int{tmp_ip3} << endl;
        } else if (amq_flag == kAmQThrottlingT2End) {
          cout << "Throttling T2 end"   << endl;
          cout << "IP: " << int{tmp_ip3} << endl;
        } else if (amq_flag == -1) {
          cerr << "Invalid header: " << hex << h_amq.head << endl;
          return -1;
        }
      }
    } else if (flag == kFileSinkTrailer) {
      flush_slice();
      ReadExact(&ifs, reinterpret_cast<char*>(&h_fst) + sizeof(uint64_t),
		sizeof(FileSinkTrailer::v1::Trailer) - sizeof(uint64_t));
      tm tm = *localtime(&h_fst.stopUnixtime);
      cout << "Stop: " << put_time(&tm, "%Y-%m-%d %H:%M:%S %Z") << endl;
    }
  }

  out_file.Write();
  return 0;
}
