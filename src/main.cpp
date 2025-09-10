#include "lob/spsc_ring.hpp"
#include "lob/event.hpp"
#include "lob/trade.hpp"
#include "lob/book.hpp"
#include "lob/matcher.hpp"
#include "lob/metrics.hpp"
#include "lob/csv.hpp"
#include <thread>
#include <fstream>
#include <iostream>
#include <atomic>
#include <vector>
#include <cstring>
#include <chrono>
using namespace lob;
struct Cli
{
  std::string in_path{"data/demo_long.csv"};
  std::string trades_path{"trades.csv"};
  std::string depth_path{"depth.csv"};
  int snapshot_every{1000};
  size_t ring_cap{1 << 20};
};
Cli parse_cli(int argc, char **argv)
{
  Cli c;
  for (int i = 1; i < argc; ++i)
  {
    if (!std::strcmp(argv[i], "--in") && i + 1 < argc)
      c.in_path = argv[++i];
    else if (!std::strcmp(argv[i], "--trades") && i + 1 < argc)
      c.trades_path = argv[++i];
    else if (!std::strcmp(argv[i], "--depth") && i + 1 < argc)
      c.depth_path = argv[++i];
    else if (!std::strcmp(argv[i], "--snapshot-every") && i + 1 < argc)
      c.snapshot_every = std::stoi(argv[++i]);
    else if (!std::strcmp(argv[i], "--ring") && i + 1 < argc)
      c.ring_cap = (size_t)std::stoul(argv[++i]);
  }
  return c;
}
int main(int argc, char **argv)
{
  Cli cli = parse_cli(argc, argv);
  SpscRing<Event> in(cli.ring_cap);
  SpscRing<Trade> out(cli.ring_cap);
  SpscRing<DepthL1> depth_out(cli.ring_cap);
  Book book;
  Metrics metrics;
  std::atomic<bool> ingest_done{false};
  std::atomic<bool> match_done{false};
  std::thread ingest([&]
                     { metrics.t0_ingest=std::chrono::steady_clock::now(); 
                      std::ifstream ifs(cli.in_path);
      if(!ifs){ 
        std::cerr<<"Failed to open input: "<<cli.in_path<<"\n"; 
        ingest_done.store(true); return; 
      }
      std::string line; 
      bool header=true; 
      while(std::getline(ifs,line)){ 
        if(header){ 
          header=false; 
          continue; 
        } 
        auto e_opt=parse_event_csv_line(line,false); 
        if(!e_opt) continue; 
        Event e=*e_opt; 
        ++metrics.input_msgs; 
        while(!in.try_push(e)){ 
          std::this_thread::yield(); 
        } }
        metrics.t1_ingest=std::chrono::steady_clock::now();
        ingest_done.store(true,std::memory_order_release);
      });
  std::thread match([&]
                    { 
                      metrics.t0_match=std::chrono::steady_clock::now(); 
                      Matcher m(in,out,depth_out,book,metrics,cli.snapshot_every); 
                      m.run(ingest_done); 
                      metrics.t1_match=std::chrono::steady_clock::now(); 
                      match_done.store(true,std::memory_order_release); 
                    });
  std::thread writer([&]
                     { metrics.t0_output=std::chrono::steady_clock::now();
                      std::ofstream trades(cli.trades_path), depth(cli.depth_path);
                      write_trades_csv_header(trades); write_depth_csv_header(depth);
    Trade t; 
    DepthL1 d; 
    bool out_empty=false, depth_empty=false;
    while(true){
      bool progressed=false;
      while(out.try_pop(t)){ 
        append_trade_csv(trades,t); 
        ++metrics.output_msgs; 
        progressed=true; 
      }
      while(depth_out.try_pop(d)){
        append_depth_csv(depth,d); 
        progressed=true;
      }
      if(!progressed){ 
        out_empty=out.empty(); 
        depth_empty=depth_out.empty();
        if(ingest_done.load(std::memory_order_acquire)&&match_done.load(std::memory_order_acquire)&&out_empty&&depth_empty) break;
        std::this_thread::yield(); 
      }
    }
    metrics.t1_output=std::chrono::steady_clock::now(); 
    trades.flush(); depth.flush();
    auto dur_s=[](auto t0,auto t1){ 
      using F=std::chrono::duration<double>; 
      return std::chrono::duration_cast<F>(t1-t0).count(); 
    };
    double ing_s=dur_s(metrics.t0_ingest,metrics.t1_ingest), mat_s=dur_s(metrics.t0_match,metrics.t1_match), out_s=dur_s(metrics.t0_output,metrics.t1_output);
    if(ing_s>0) 
      metrics.ingest_msgs_per_s=metrics.input_msgs/ing_s; 
    if(mat_s>0) 
      metrics.match_msgs_per_s=metrics.matched_msgs/mat_s; 
    if(out_s>0) 
      metrics.output_msgs_per_s=metrics.output_msgs/out_s;
    double p50_ns=metrics.match_latency_us.pct_ns(0.50);
    double p95_ns=metrics.match_latency_us.pct_ns(0.95);
    double p99_ns=metrics.match_latency_us.pct_ns(0.99);
    double   p50_us = p50_ns / 1000.0;
    double   p95_us = p95_ns / 1000.0;
    double   p99_us = p99_ns / 1000.0;
    std::ofstream mjson("metrics.json"); mjson<<"{\n"
      <<"  \"input_msgs\": "<<metrics.input_msgs<<",\n"
      <<"  \"matched_msgs\": "<<metrics.matched_msgs<<",\n"
      <<"  \"output_msgs\": "<<metrics.output_msgs<<",\n"
      <<"  \"snapshot_count\": "<<metrics.snapshot_count<<",\n"
      <<"  \"ingest_msgs_per_s\": "<<metrics.ingest_msgs_per_s<<",\n"
      <<"  \"match_msgs_per_s\": "<<metrics.match_msgs_per_s<<",\n"
      <<"  \"output_msgs_per_s\": "<<metrics.output_msgs_per_s<<",\n"
      <<"  \"match_p50_us\": "<<p50_ns<<",\n"
      <<"  \"match_p95_us\": "<<p95_ns<<",\n"
      <<"  \"match_p99_us\": "<<p99_ns<<"\n"
      << "  \"match_p50_us\": " << p50_us << ",\n"
      << "  \"match_p95_us\": " << p95_us << ",\n"
      << "  \"match_p99_us\": " << p99_us << "\n"
      <<"}\n"; });
  ingest.join();
  match.join();
  writer.join();
  return 0;
}