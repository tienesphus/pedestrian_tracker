#ifndef LIBBUSCOUNT_HPP
#define LIBBUSCOUNT_HPP

class BusCounter {
  std::function<bool(Ptr<Mat>)> _src;
  std::function<flow::tuple<Ptr<WorldState>, Ptr<Mat>>(const Ptr<WorldState>)> _dest;
public:
  BusCounter(
    std::function<bool(Ptr<Mat>)> src,
    std::function<flow::tuple<Ptr<WorldState>, Ptr<Mat>>(const Ptr<WorldState>)> dest
  );
  void run(const NetConfigIR &net_config, const WorldConfig world_config, bool draw);
};

#endif
