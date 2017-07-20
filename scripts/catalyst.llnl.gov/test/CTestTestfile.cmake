# CMake generated Testfile for 
# Source directory: /g/g91/zhang50/havoqgt/test
# Build directory: /g/g91/zhang50/havoqgt/scripts/catalyst.llnl.gov/test
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
ADD_TEST(sequential_nompi "test_sequential")
ADD_TEST(mpi_communicator_2 "/usr/bin/srun" "-np" "2" "/g/g91/zhang50/havoqgt/scripts/catalyst.llnl.gov/test/test_mpi_communicator")
ADD_TEST(mpi_communicator_4 "/usr/bin/srun" "-np" "4" "/g/g91/zhang50/havoqgt/scripts/catalyst.llnl.gov/test/test_mpi_communicator")
ADD_TEST(delegate_graph_static_2 "/usr/bin/srun" "-np" "2" "/g/g91/zhang50/havoqgt/scripts/catalyst.llnl.gov/test/test_delegate_graph_static")
ADD_TEST(delegate_graph_static_4 "/usr/bin/srun" "-np" "4" "/g/g91/zhang50/havoqgt/scripts/catalyst.llnl.gov/test/test_delegate_graph_static")
ADD_TEST(static_bfs_2 "/usr/bin/srun" "-np" "2" "/g/g91/zhang50/havoqgt/scripts/catalyst.llnl.gov/test/test_static_bfs")
ADD_TEST(static_bfs_4 "/usr/bin/srun" "-np" "4" "/g/g91/zhang50/havoqgt/scripts/catalyst.llnl.gov/test/test_static_bfs")
ADD_TEST(copy_2 "/usr/bin/srun" "-np" "2" "/g/g91/zhang50/havoqgt/scripts/catalyst.llnl.gov/test/test_copy")
ADD_TEST(copy_4 "/usr/bin/srun" "-np" "4" "/g/g91/zhang50/havoqgt/scripts/catalyst.llnl.gov/test/test_copy")
SUBDIRS(gtest)
SUBDIRS(include)
