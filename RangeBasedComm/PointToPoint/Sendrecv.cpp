/*****************************************************************************
 * This file is part of the Project SchizophrenicQuicksort
 * 
 * Copyright (c) 2016-2017, Armin Wiebigke <armin.wiebigke@gmail.com>
 * Copyright (c) 2016-2017, Michael Axtmann <michael.axtmann@kit.edu>
 *
 * All rights reserved. Published under the BSD-2 license in the LICENSE file.
 ******************************************************************************/

#include <mpi.h>

#include "../RBC.hpp"

namespace RBC {

    int Sendrecv(void *sendbuf,
            int sendcount, MPI_Datatype sendtype,
            int dest, int sendtag,
            void *recvbuf, int recvcount, MPI_Datatype recvtype,
            int source, int recvtag,
            Comm const &comm, MPI_Status *status) {
        return MPI_Sendrecv(sendbuf,
                sendcount, sendtype,
                comm.RangeRankToMpiRank(dest), sendtag,
                recvbuf, recvcount, recvtype,
                comm.RangeRankToMpiRank(source), recvtag,
                comm.mpi_comm, status);
    }

    namespace _internal {

        /*
         * Request for the isend
         */
        class IsendrecvReq : public RequestSuperclass {
        public:
            IsendrecvReq(void *sendbuf,
                    int sendcount, MPI_Datatype sendtype, int dest, int sendtag,
                    void *recvbuf, int recvcount, MPI_Datatype recvtype,
                    int source, int recvtag,
                    Comm const &comm);
            int test(int *flag, MPI_Status *status);

        private:
            MPI_Request requests[2];
        };

        

        /*
         * Sendrecv operation which drops empty messages.
         */
        int SendrecvNonZeroed(void *sendbuf,
                int sendcount, MPI_Datatype sendtype,
                int dest, int sendtag,
                void *recvbuf, int recvcount, MPI_Datatype recvtype,
                int source, int recvtag,
                Comm const &comm, MPI_Status *status) {
            if (sendcount > 0 && recvcount > 0) {
            return MPI_Sendrecv(sendbuf,
                    sendcount, sendtype,
                    comm.RangeRankToMpiRank(dest), sendtag,
                    recvbuf, recvcount, recvtype,
                    comm.RangeRankToMpiRank(source), recvtag,
                    comm.mpi_comm, status);
            } else if (sendcount > 0) {
                RBC::Send(sendbuf, sendcount, sendtype, dest, sendtag, comm);
            } else if (recvcount > 0) {
                RBC::Recv(recvbuf, recvcount, recvtype, source, recvtag, comm, MPI_STATUS_IGNORE);
            }

            // Case: Both messages are empty.
            return 0;
        }
    } // end namespace _internal

    int Isendrecv(void *sendbuf,
            int sendcount, MPI_Datatype sendtype,
            int dest, int sendtag,
            void *recvbuf, int recvcount, MPI_Datatype recvtype,
            int source, int recvtag,
            Comm const &comm, Request *request) {
        request->set(std::make_shared<_internal::IsendrecvReq>(sendbuf,
                sendcount, sendtype, dest, sendtag, recvbuf, recvcount, recvtype,
                source, recvtag, comm));
        return 0;
    };
}

RBC::_internal::IsendrecvReq::IsendrecvReq(void *sendbuf,
        int sendcount, MPI_Datatype sendtype, int dest, int sendtag,
        void *recvbuf, int recvcount, MPI_Datatype recvtype,
        int source, int recvtag,
        RBC::Comm const &comm) {
    void* sendbuf_ = const_cast<void*> (sendbuf);
    MPI_Isend(sendbuf_, sendcount, sendtype, comm.RangeRankToMpiRank(dest),
            sendtag, comm.mpi_comm, &requests[0]);
    MPI_Irecv(recvbuf, recvcount, recvtype, comm.RangeRankToMpiRank(source),
            recvtag, comm.mpi_comm, &requests[1]);
};

int RBC::_internal::IsendrecvReq::test(int *flag, MPI_Status *) {
    return MPI_Testall(2, requests, flag, MPI_STATUSES_IGNORE);
};
