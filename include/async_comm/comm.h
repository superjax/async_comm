/*
 * Software License Agreement (BSD-3 License)
 *
 * Copyright (c) 2018 Daniel Koch.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file comm.h
 * @author Daniel Koch <danielpkoch@gmail.com>
 */

#ifndef ASYNC_COMM_COMM_H
#define ASYNC_COMM_COMM_H

#include <functional>
#include <list>
#include <mutex>
#include <thread>

#include <boost/asio.hpp>
#include <boost/function.hpp>

namespace async_comm
{

/**
 * @class Comm
 * @brief Abstract base class for an asynchronous communication port
 */
class Comm
{
public:
  Comm();
  virtual ~Comm();

  /**
   * @brief Initializes and opens the port
   * @return True if the port was succesfully initialized
   */
  bool init();

  /**
   * @brief Closes the port
   */
  void close();

  /**
   * @brief Send bytes from a buffer over the port
   * @param src Address of the buffer
   * @param len Number of bytes to send
   */
  void send_bytes(const uint8_t * src, size_t len);

  /**
   * @brief Register a callback function for when bytes are received on the port
   * @param fun Function to call when bytes are received
   */
  void register_receive_callback(std::function<void(uint8_t)> fun);

protected:
  static constexpr size_t READ_BUFFER_SIZE = 512;
  static constexpr size_t MAX_PACKET_LEN = 256;

  virtual bool is_open() = 0;
  virtual bool do_init() = 0;
  virtual void do_close() = 0;
  virtual void do_async_read(const boost::asio::mutable_buffers_1 &buffer,
                             boost::function<void(const boost::system::error_code&, size_t)> handler) = 0;
  virtual void do_async_write(const boost::asio::const_buffers_1 &buffer,
                              boost::function<void(const boost::system::error_code&, size_t)> handler) = 0;

  boost::asio::io_service io_service_;

private:

  struct WriteBuffer
  {
    uint8_t data[MAX_PACKET_LEN];
    size_t len;
    size_t pos;

    WriteBuffer() : len(0), pos(0) {}

    WriteBuffer(const uint8_t * buf, uint16_t len) : len(len), pos(0)
    {
      assert(len <= MAX_PACKET_LEN); //! \todo Do something less catastrophic here
      memcpy(data, buf, len);
    }

    const uint8_t * dpos() const { return data + pos; }

    size_t nbytes() const { return len - pos; }
  };

  typedef std::lock_guard<std::recursive_mutex> mutex_lock;
  void async_read();
  void async_read_end(const boost::system::error_code& error, size_t bytes_transferred);

  void async_write(bool check_write_state);
  void async_write_end(const boost::system::error_code& error, size_t bytes_transferred);

  std::thread io_thread_;
  std::recursive_mutex mutex_;

  uint8_t read_buffer_[READ_BUFFER_SIZE];
  std::list<WriteBuffer*> write_queue_;
  bool write_in_progress_;

  std::function<void(uint8_t)> receive_callback_;
};

} // namespace async_comm

#endif // ASYNC_COMM_COMM_H