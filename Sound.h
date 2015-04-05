#ifndef SoundH
#define SoundH

#include <map>
#include <cstdint>

using ALuint = std::uint32_t;

class TSoundsManager
{
  private:
    std::map<std::string, ALuint> sources_;

    TSoundsManager() = default;
    ~TSoundsManager() = default;

  public:
    static ALuint GetFromName(const std::string name, bool dynamic, float *fSamplingRate = NULL);
};

#endif
