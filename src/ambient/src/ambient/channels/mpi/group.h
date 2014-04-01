/*
 * Ambient, License - Version 1.0 - May 3rd, 2012
 *
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef AMBIENT_CHANNELS_MPI_GROUP
#define AMBIENT_CHANNELS_MPI_GROUP

namespace ambient { namespace channels { namespace mpi {

    class group
    {
    public:
        ~group();
        group(int master, MPI_Comm parent); // global group
        group(int master, group* parent);   // nested group
        void commit();

        size_t get_proc(size_t k);

        /*void add(const int* procs, int count);
        void add(int count); // loose method
        void add_range(int first, int last);
        void add_every(int nth);
        void add_every(bool(*include)(int k));
        void add_intersection(const group* b, int* count = NULL);
        void add_every_intersection(const group* b, int nth, int* count = NULL);
        void add_substraction(const group* b, int* count = NULL);*/
        void reorder(int(*permutation)(int r));
        void reorder(int* permutation);

        int get_vacant();    // get the vacant rank for the child creation
        bool occupied();
        void occupy();
        void idle();

        int  vacant_level;
        int* vacations;
        bool occupancy;

        int  id;
        int  rank;
        int  master;                // master process in this group
        int* ranks;                 // member ranks in the parent group
        int* procs;                 // member ranks in the world group
        int  size;                  // number of processes inside group
        int  depth;
        group* parent;              // parent group of processes
        std::set<group*> children;

        MPI_Comm  mpi_comm;
        MPI_Group mpi_group;
    };

    // group* group_map(size_t id, group* instance = NULL);

} } }

#endif
