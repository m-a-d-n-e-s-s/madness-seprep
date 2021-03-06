/*
  This file is part of MADNESS.

  Copyright (C) 2007,2010 Oak Ridge National Laboratory

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

  For more information please contact:

  Robert J. Harrison
  Oak Ridge National Laboratory
  One Bethel Valley Road
  P.O. Box 2008, MS-6367

  email: harrisonrj@ornl.gov
  tel:   865-241-3937
  fax:   865-572-0680
*/

#ifndef MADNESS_WORLD_WORLDRMI_H__INCLUDED
#define MADNESS_WORLD_WORLDRMI_H__INCLUDED

#include <madness/world/safempi.h>
#include <madness/world/thread.h>
#include <madness/world/worldtypes.h>
#include <sstream>
#include <utility>
#include <list>
#include <memory>

/*
  There is just one server thread and it is the only one
  messing with the recv buffers, so there is no need for
  mutex on recv related data.

  Multiple threads (including the server) may send hence
  we need to be careful about send-related data.

  When MPI is initialized we need to use init_thread with
  multiple required.

  This RMI service operates only in COMM_WORLD.  It easy enough
  to extend to other communicators but the point is to have
  only one server thread for all possible uses.  You just
  have to translate rank_in_comm into rank_in_world by
  getting the groups from both communicators using
  MPI_Comm_group and then creating a map from ranks in
  comm to ranks in world using MPI_Group_translate_ranks.

  The class is a singleton ... i.e., there is only one instance of it
  that is made the first time that you call RMI::instance().

  Handler routines should have this type

  typedef void (*rmi_handlerT)(void* buf, size_t nbyte);

  There are few user accessible routines.

  RMI::Request RMI::isend(const void* buf, size_t nbyte, int dest,
                          rmi_handlerT func, unsigned int attr=0)
  - to send an asynchronous message
  - RMI::Request has the same interface as SafeMPI::Request
  (right now it is a SafeMPI::Request but this is not guaranteed)

  void RMI::begin()
  - to start the server thread

  void RMI::end()
  - to terminate the server thread

  bool RMI::get_debug()
  - to get the debug flag

  void RMI::set_debug(bool)
  - to set the debug flag

*/

/**
 \file worldrmi.h
 \brief Lowest level API for sending active messages --- you should probably be looking at worldam.h instead.
 \addtogroup parallel_runtime
 */

namespace madness {

    /// This is the generic low-level interface for a message handler
    typedef void (*rmi_handlerT)(void* buf, size_t nbyte);

    struct qmsg {
        typedef uint16_t counterT;
        typedef uint32_t attrT;
        size_t len;
        rmi_handlerT func;
        int i;               // buffer index
        ProcessID src;
        attrT attr;
        counterT count;

        qmsg(size_t len, rmi_handlerT func, int i, int src, attrT attr, counterT count)
            : len(len), func(func), i(i), src(src), attr(attr), count(count) {}

        bool operator<(const qmsg& other) const {
            return count < other.count;
        }

        qmsg() {}
    }; // struct qmsg


    // Holds message passing statistics
    struct RMIStats {
        uint64_t nmsg_sent;
        uint64_t nbyte_sent;
        uint64_t nmsg_recv;
        uint64_t nbyte_recv;

        RMIStats()
                : nmsg_sent(0), nbyte_sent(0), nmsg_recv(0), nbyte_recv(0) {}
    };


    /// This class implements the communications server thread and provides the only send interface
    class RMI  {
        typedef uint16_t counterT;
        typedef uint32_t attrT;
    public:

        typedef SafeMPI::Request Request;

        // Choose header length to hold at least sizeof(header) and
        // also to ensure good alignment of the user payload.
        static const size_t ALIGNMENT = 64;
        static const size_t HEADER_LEN = ALIGNMENT;
        static const attrT ATTR_UNORDERED=0x0;
        static const attrT ATTR_ORDERED=0x1;

        static int testsome_backoff_us;

    private:

        class RmiTask
#if HAVE_INTEL_TBB
                : public tbb::task, private madness::Mutex
#else
                : public madness::ThreadBase, private madness::Mutex
#endif // HAVE_INTEL_TBB
        {

        public:

            struct header {
                rmi_handlerT func;
                attrT attr;
            }; // struct header

            std::list< std::pair<int,size_t> > hugeq; // q for incoming huge messages

            SafeMPI::Intracomm comm;
            const int nproc;            // No. of processes in comm world
            const ProcessID rank;       // Rank of this process
            volatile bool finished;     // True if finished

            std::unique_ptr<volatile counterT[]> send_counters;
            std::unique_ptr<counterT[]> recv_counters;
            std::size_t max_msg_len_;
            std::size_t nrecv_;
            std::size_t maxq_;
            std::unique_ptr<void*[]> recv_buf; // Will be at least ALIGNMENT aligned ... +1 for huge messages /// \todo Are [] needed in the template?
            std::unique_ptr<SafeMPI::Request[]> recv_req;

            std::unique_ptr<SafeMPI::Status[]> status;
            std::unique_ptr<int[]> ind;
            std::unique_ptr<qmsg[]> q;
            int n_in_q;

            static inline bool is_ordered(attrT attr) { return attr & ATTR_ORDERED; }

            void process_some();

            RmiTask();
            virtual ~RmiTask();

#if HAVE_INTEL_TBB
            tbb::task* execute() {
                // Process some messages
                process_some();
                if(! finished) {
                   tbb::task::increment_ref_count();
                   tbb::task::recycle_as_safe_continuation();
                } else {
                    finished = false;
                }
                return nullptr;
            }
#else
            void run() {
                try {
                    while (! finished) process_some();
                    finished = false;
                } catch(...) {
                    delete this;
                    throw;
                }
                delete this;
            }
#endif // HAVE_INTEL_TBB

            void exit() {
                if (debugging)
                    std::cerr << rank << ":RMI: sending exit request to server thread" << std::endl;

                // Set finished flag
                finished = true;
                while(finished)
                    myusleep(1000);
            }

            static void huge_msg_handler(void *buf, size_t nbytein);

            Request isend(const void* buf, size_t nbyte, ProcessID dest, rmi_handlerT func, attrT attr);

            void post_pending_huge_msg();

            void post_recv_buf(int i);

        }; // class RmiTask

#if HAVE_INTEL_TBB
        static tbb::task* tbb_rmi_parent_task;
#endif // HAVE_INTEL_TBB

        static RmiTask* task_ptr;    // Pointer to the singleton instance
        static RMIStats stats;
        static volatile bool debugging;    // True if debugging

        static const size_t DEFAULT_MAX_MSG_LEN = 3*512*1024;
        static const int DEFAULT_NRECV = 128;

        // Not allowed
        RMI(const RMI&);
        RMI& operator=(const RMI&);

    public:

        static std::size_t max_msg_len() {
            return (task_ptr ? task_ptr->max_msg_len_ : DEFAULT_MAX_MSG_LEN);
        }
        static std::size_t maxq() {
            MADNESS_ASSERT(task_ptr);
            return task_ptr->maxq_;
        }
        static std::size_t nrecv() {
            MADNESS_ASSERT(task_ptr);
            return task_ptr->nrecv_;
        }

        /// Send a remote method invocation (again you should probably be looking at worldam.h instead)

        /// @param[in] buf Pointer to the data buffer (do not modify until send is completed)
        /// @param[in] nbyte Size of the data in bytes
        /// @param[in] dest Process to receive the message
        /// @param[in] func The function to handle the message on the remote end
        /// @param[in] attr Attributes of the message (ATTR_UNORDERED or ATTR_ORDERED)
        /// @return The status as an RMI::Request that presently is a SafeMPI::Request
        static Request
        isend(const void* buf, size_t nbyte, ProcessID dest, rmi_handlerT func, unsigned int attr=ATTR_UNORDERED) {
            if(!task_ptr) {
              std::cerr <<
                  "!! MADNESS RMI error: Attempting to send a message when the RMI thread is not running\n"
                  "!! MADNESS RMI error: This typically occurs when an active message is sent or a remote task is spawned after calling madness::finalize()\n";
              MADNESS_EXCEPTION("!! MADNESS error: The RMI thread is not running", (task_ptr != nullptr));
            }
            return task_ptr->isend(buf, nbyte, dest, func, attr);
        }

        static void begin() {
            testsome_backoff_us = 5;
            const char* buf = getenv("MAD_BACKOFF_US");
            if (buf) {
                std::stringstream ss(buf);
                ss >> testsome_backoff_us;
                if (testsome_backoff_us < 0) testsome_backoff_us = 0;
                if (testsome_backoff_us > 100) testsome_backoff_us = 100;
            }

            MADNESS_ASSERT(task_ptr == nullptr);
#if HAVE_INTEL_TBB
            tbb_rmi_parent_task = new( tbb::task::allocate_root() ) tbb::empty_task;
            tbb_rmi_parent_task->set_ref_count(2);

            task_ptr = new( tbb_rmi_parent_task->allocate_child() ) RmiTask();
            tbb::task::enqueue(*task_ptr, tbb::priority_high);
#else
            task_ptr = new RmiTask();
            task_ptr->start();
#endif // HAVE_INTEL_TBB
        }

        static void end() {
            if(task_ptr) {
                task_ptr->exit();
#if HAVE_INTEL_TBB
                tbb_rmi_parent_task->wait_for_all();
                tbb::task::destroy(*tbb_rmi_parent_task);
#endif // HAVE_INTEL_TBB
                task_ptr = nullptr;
            }
        }

        static void set_debug(bool status) { debugging = status; }

        static bool get_debug() { return debugging; }

        static const RMIStats& get_stats() { return stats; }
    }; // class RMI

} // namespace madness

#endif // MADNESS_WORLD_WORLDRMI_H__INCLUDED
