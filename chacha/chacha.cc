// Dedicated to the Public Domain under the Unlicense: https://unlicense.org/UNLICENSE

#include <array>
#include <cstdint>
#include <cassert>
#include <cstring>

#include "chacha.hh"

#include <vector>
#include <cstdio>
#include <ctime>
#include <string>
#include <algorithm>
#include <sys/random.h>

static std::string
state_to_string (const std::array<uint32_t, 16> &s)
{
  char cb[4096];
  snprintf (cb, sizeof (cb),
            "%08x  %08x  %08x  %08x\n" "%08x  %08x  %08x  %08x\n" "%08x  %08x  %08x  %08x\n" "%08x  %08x  %08x  %08x",
            s[0], s[1], s[2], s[3],  s[4], s[5], s[6], s[7],  s[8], s[9], s[10],s[11],  s[12],s[13],s[14],s[15]);
  return cb;
}

static std::string
hex_to_string (const uint8_t *d, unsigned l, unsigned step = 16)
{
  std::string s;
  const unsigned B = 4096;
  char b[B];
  for (size_t i = 0; i < l; i++) {
    if (0 == (i & (step-1))) {
      snprintf (b, B, "%03zd ", i);
      s += b;
    }
    snprintf (b, B, " %02x", d[i]);
    s += b;
    if (0 == ((i+1) & (step-1)))
      s += "\n";
  }
  if (!s.empty() && s[s.size()-1] != '\n')
    s += "\n";
  return s;
}

static void
chacha_tests ()
{
  struct TestVector {
    std::array<uint8_t, 32> key;
    std::array<uint8_t, 12> nonce;
    uint32_t                counter;
    std::vector<uint8_t>    plaintext;
    std::vector<uint8_t>    expected;
  };
  static const TestVector tests[] = {
    // RFC7539 2.6.2.
    { { 0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
        0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f },
      { 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 },
      0x00000000, {},
      { 0x8a, 0xd5, 0xa0, 0x8b, 0x90, 0x5f, 0x81, 0xcc, 0x81, 0x50, 0x40, 0x27, 0x4a, 0xb2, 0x94, 0x71,
        0xa8, 0x33, 0xb6, 0x37, 0xe3, 0xfd, 0x0d, 0xa5, 0x08, 0xdb, 0xb8, 0xe2, 0xfd, 0xd1, 0xa6, 0x46 } },
    // RFC7539 A.1. #1
    { { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
      { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
      0x00000000, {},
      { 0x76, 0xb8, 0xe0, 0xad, 0xa0, 0xf1, 0x3d, 0x90, 0x40, 0x5d, 0x6a, 0xe5, 0x53, 0x86, 0xbd, 0x28,
        0xbd, 0xd2, 0x19, 0xb8, 0xa0, 0x8d, 0xed, 0x1a, 0xa8, 0x36, 0xef, 0xcc, 0x8b, 0x77, 0x0d, 0xc7,
        0xda, 0x41, 0x59, 0x7c, 0x51, 0x57, 0x48, 0x8d, 0x77, 0x24, 0xe0, 0x3f, 0xb8, 0xd8, 0x4a, 0x37,
        0x6a, 0x43, 0xb8, 0xf4, 0x15, 0x18, 0xa1, 0x1c, 0xc3, 0x87, 0xb6, 0x69, 0xb2, 0xee, 0x65, 0x86 } },
    // RFC7539 A.1. #2
    { { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
      { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
      0x00000001, {},
      { 0x9f, 0x07, 0xe7, 0xbe, 0x55, 0x51, 0x38, 0x7a, 0x98, 0xba, 0x97, 0x7c, 0x73, 0x2d, 0x08, 0x0d,
        0xcb, 0x0f, 0x29, 0xa0, 0x48, 0xe3, 0x65, 0x69, 0x12, 0xc6, 0x53, 0x3e, 0x32, 0xee, 0x7a, 0xed,
        0x29, 0xb7, 0x21, 0x76, 0x9c, 0xe6, 0x4e, 0x43, 0xd5, 0x71, 0x33, 0xb0, 0x74, 0xd8, 0x39, 0xd5,
        0x31, 0xed, 0x1f, 0x28, 0x51, 0x0a, 0xfb, 0x45, 0xac, 0xe1, 0x0a, 0x1f, 0x4b, 0x79, 0x4d, 0x6f } },
    // RFC7539 A.1. #3
    { { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 },
      { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
      0x00000001, {},
      { 0x3a, 0xeb, 0x52, 0x24, 0xec, 0xf8, 0x49, 0x92, 0x9b, 0x9d, 0x82, 0x8d, 0xb1, 0xce, 0xd4, 0xdd,
        0x83, 0x20, 0x25, 0xe8, 0x01, 0x8b, 0x81, 0x60, 0xb8, 0x22, 0x84, 0xf3, 0xc9, 0x49, 0xaa, 0x5a,
        0x8e, 0xca, 0x00, 0xbb, 0xb4, 0xa7, 0x3b, 0xda, 0xd1, 0x92, 0xb5, 0xc4, 0x2f, 0x73, 0xf2, 0xfd,
        0x4e, 0x27, 0x36, 0x44, 0xc8, 0xb3, 0x61, 0x25, 0xa6, 0x4a, 0xdd, 0xeb, 0x00, 0x6c, 0x13, 0xa0 } },
    // RFC7539 A.1. #4
    { { 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
      { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
      0x00000002, {},
      { 0x72, 0xd5, 0x4d, 0xfb, 0xf1, 0x2e, 0xc4, 0x4b, 0x36, 0x26, 0x92, 0xdf, 0x94, 0x13, 0x7f, 0x32,
        0x8f, 0xea, 0x8d, 0xa7, 0x39, 0x90, 0x26, 0x5e, 0xc1, 0xbb, 0xbe, 0xa1, 0xae, 0x9a, 0xf0, 0xca,
        0x13, 0xb2, 0x5a, 0xa2, 0x6c, 0xb4, 0xa6, 0x48, 0xcb, 0x9b, 0x9d, 0x1b, 0xe6, 0x5b, 0x2c, 0x09,
        0x24, 0xa6, 0x6c, 0x54, 0xd5, 0x45, 0xec, 0x1b, 0x73, 0x74, 0xf4, 0x87, 0x2e, 0x99, 0xf0, 0x96 } },
    // RFC7539 A.1. #5
    { { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
      { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02 },
      0x00000000, {},
      { 0xc2, 0xc6, 0x4d, 0x37, 0x8c, 0xd5, 0x36, 0x37, 0x4a, 0xe2, 0x04, 0xb9, 0xef, 0x93, 0x3f, 0xcd,
        0x1a, 0x8b, 0x22, 0x88, 0xb3, 0xdf, 0xa4, 0x96, 0x72, 0xab, 0x76, 0x5b, 0x54, 0xee, 0x27, 0xc7,
        0x8a, 0x97, 0x0e, 0x0e, 0x95, 0x5c, 0x14, 0xf3, 0xa8, 0x8e, 0x74, 0x1b, 0x97, 0xc2, 0x86, 0xf7,
        0x5f, 0x8f, 0xc2, 0x99, 0xe8, 0x14, 0x83, 0x62, 0xfa, 0x19, 0x8a, 0x39, 0x53, 0x1b, 0xed, 0x6d } },
    // RFC8439 2.3.2.
    { { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f },
      { 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x4a, 0x00, 0x00, 0x00, 0x00 },
      0x00000001, {},
      { 0x10, 0xf1, 0xe7, 0xe4, 0xd1, 0x3b, 0x59, 0x15, 0x50, 0x0f, 0xdd, 0x1f, 0xa3, 0x20, 0x71, 0xc4,
        0xc7, 0xd1, 0xf4, 0xc7, 0x33, 0xc0, 0x68, 0x03, 0x04, 0x22, 0xaa, 0x9a, 0xc3, 0xd4, 0x6c, 0x4e,
        0xd2, 0x82, 0x64, 0x46, 0x07, 0x9f, 0xaa, 0x09, 0x14, 0xc2, 0xd7, 0x05, 0xd9, 0x8b, 0x02, 0xa2,
        0xb5, 0x12, 0x9c, 0xd1, 0xde, 0x16, 0x4e, 0xb9, 0xcb, 0xd0, 0x83, 0xe8, 0xa2, 0x50, 0x3c, 0x4e } },
    // RFC8439 2.4.2.
    { { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f },
      { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4a, 0x00, 0x00, 0x00, 0x00 },
      0x00000001,
      { 0x4c, 0x61, 0x64, 0x69, 0x65, 0x73, 0x20, 0x61, 0x6e, 0x64, 0x20, 0x47, 0x65, 0x6e, 0x74, 0x6c,
        0x65, 0x6d, 0x65, 0x6e, 0x20, 0x6f, 0x66, 0x20, 0x74, 0x68, 0x65, 0x20, 0x63, 0x6c, 0x61, 0x73,
        0x73, 0x20, 0x6f, 0x66, 0x20, 0x27, 0x39, 0x39, 0x3a, 0x20, 0x49, 0x66, 0x20, 0x49, 0x20, 0x63,
        0x6f, 0x75, 0x6c, 0x64, 0x20, 0x6f, 0x66, 0x66, 0x65, 0x72, 0x20, 0x79, 0x6f, 0x75, 0x20, 0x6f,
        0x6e, 0x6c, 0x79, 0x20, 0x6f, 0x6e, 0x65, 0x20, 0x74, 0x69, 0x70, 0x20, 0x66, 0x6f, 0x72, 0x20,
        0x74, 0x68, 0x65, 0x20, 0x66, 0x75, 0x74, 0x75, 0x72, 0x65, 0x2c, 0x20, 0x73, 0x75, 0x6e, 0x73,
        0x63, 0x72, 0x65, 0x65, 0x6e, 0x20, 0x77, 0x6f, 0x75, 0x6c, 0x64, 0x20, 0x62, 0x65, 0x20, 0x69,
        0x74, 0x2e },
      { 0x6e, 0x2e, 0x35, 0x9a, 0x25, 0x68, 0xf9, 0x80, 0x41, 0xba, 0x07, 0x28, 0xdd, 0x0d, 0x69, 0x81,
        0xe9, 0x7e, 0x7a, 0xec, 0x1d, 0x43, 0x60, 0xc2, 0x0a, 0x27, 0xaf, 0xcc, 0xfd, 0x9f, 0xae, 0x0b,
        0xf9, 0x1b, 0x65, 0xc5, 0x52, 0x47, 0x33, 0xab, 0x8f, 0x59, 0x3d, 0xab, 0xcd, 0x62, 0xb3, 0x57,
        0x16, 0x39, 0xd6, 0x24, 0xe6, 0x51, 0x52, 0xab, 0x8f, 0x53, 0x0c, 0x35, 0x9f, 0x08, 0x61, 0xd8,
        0x07, 0xca, 0x0d, 0xbf, 0x50, 0x0d, 0x6a, 0x61, 0x56, 0xa3, 0x8e, 0x08, 0x8a, 0x22, 0xb6, 0x5e,
        0x52, 0xbc, 0x51, 0x4d, 0x16, 0xcc, 0xf8, 0x06, 0x81, 0x8c, 0xe9, 0x1a, 0xb7, 0x79, 0x37, 0x36,
        0x5a, 0xf9, 0x0b, 0xbf, 0x74, 0xa3, 0x5b, 0xe6, 0xb4, 0x0b, 0x8e, 0xed, 0xf2, 0x78, 0x5e, 0x42,
        0x87, 0x4d } },
  };
  std::vector<uint8_t> testbuf, plaintext;
  for (size_t i = 0; i < sizeof (tests) / sizeof (tests[0]); i++) {
    const auto &T = tests[i];
    std::array<uint32_t, 16> state;
    ChaCha::rfc7539_setup (state, T.key, T.nonce, T.counter);
    const auto initstr = state_to_string (state);
    plaintext = T.plaintext;
    if (T.plaintext.size() < T.expected.size())
      plaintext.resize (T.expected.size(), 0);
    testbuf.assign (T.expected.size(), 0x00);
    ChaCha::encrypt (state, T.expected.size(), plaintext.data(), testbuf.data(), 20);
    if (0)
      printf ("init: (Test #%zu)\n%s\nresult: (bytes=%zu)\n%sexpected: (bytes=%zu)\n%safter:\n%s\n", i, initstr.c_str(),
              testbuf.size(), hex_to_string (testbuf.data(), testbuf.size()).c_str(),
              T.expected.size(), hex_to_string (T.expected.data(), T.expected.size()).c_str(),
              state_to_string (state).c_str());
    assert (testbuf.size() == T.expected.size());
    for (size_t j = 0; j < T.expected.size(); j++) {
      // dprintf (2, "#%zu) [%zu]: %02x == %02x\n", i, j, T.expected[j], testbuf[j]);
      assert (testbuf[j] == T.expected[j]);
    }
    dprintf (2, "  OK    (#%zu, bytes=%zu)\n", i, T.expected.size());
  }
}

static void
chacha_stream_tests (uint64_t nonce, const std::array<uint8_t, 32> &key)
{
  std::array<uint32_t, 16> state;
  ChaCha::key_setup (state, 256, key, nonce);
  std::array<uint32_t, 16> orig_state (state);
  const unsigned N = 12 * 1024 * 1024;
  std::vector<uint8_t> buffer (N, 0);
  for (size_t i = 0; i < buffer.size(); /**/) {
    ChaCha::alu_block (state, buffer.data() + i, buffer.data() + i, 20);
    i += 64;
  }
  const std::vector<uint8_t> orig (buffer);
  if (ChaCha::sse_blocks) {
    buffer.assign (N, 0);
    assert (orig != buffer);
    state = orig_state;
    for (size_t i = 0; i < buffer.size(); /**/) {
      ChaCha::sse_block (state, buffer.data() + i, buffer.data() + i, 20);
      i += ChaCha::sse_blocks * 64;
    }
    assert (orig == buffer);
    printf ("  OK    (SSE3 validation)\n");
  }
  if (ChaCha::avx_blocks) {
    buffer.assign (N, 0);
    assert (orig != buffer);
    state = orig_state;
    for (size_t i = 0; i < buffer.size(); /**/) {
      ChaCha::avx2_block (state, buffer.data() + i, buffer.data() + i, 20);
      i += ChaCha::avx_blocks * 64;
    }
    assert (orig == buffer);
    printf ("  OK    (AVX2 validation)\n");
  }
}
