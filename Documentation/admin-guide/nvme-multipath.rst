.. SPDX-License-Identifier: GPL-2.0

====================
Linux NVMe multipath
====================

This document describes NVMe multipath and its path selection policies supported
by the Linux NVMe host driver.


Introduction
============

The NVMe multipath feature in Linux integrates namespaces with the same
identifier into a single block device. Using multipath enhances the reliability
and stability of I/O access while improving bandwidth performance. When a user
sends I/O to this merged block device, the multipath mechanism selects one of
the underlying block devices (paths) according to the configured policy.
Different policies result in different path selections.


Policies
========

All policies follow the ANA (Asymmetric Namespace Access) mechanism, meaning
that when an optimized path is available, it will be chosen over a non-optimized
one. Current the NVMe multipath policies include numa(default), round-robin and
queue-depth.

To set the desired policy (e.g., round-robin), use one of the following methods:
   1. echo -n "round-robin" > /sys/module/nvme_core/parameters/iopolicy
   2. or add the "nvme_core.iopolicy=round-robin" to cmdline.

FC-NVMe
----
On NVMe over Fibre Channel there is an additional state called `marginal`
All policies will treat this path as offline and will not use it unless
all other paths are marginal or unavailable. In that case, the policy will
select the best path based on the policy.

To see the current path state, use one of the following methods:
   1. cat /sys/class/fc_host/host*/device/rport-*/fc_remote_ports/rport-*/port_state
   2. cat /sys/devices/virtual/nvme-subsystem/nvme-subsys*/nvme*/state
To reset a path to online use:
   1. echo "Online" > /sys/class/fc_host/host*/device/rport-*/fc_remote_ports/rport-*/port_state

The marginal state is caused by a
FPIN-LI (Fabric Performance Impact Notification - Link Impact) event.
FPIN-LI event counters can be seen with:
   1. cat /sys/class/fc_host/host*/statistics/fpin_li*

NUMA
----

The NUMA policy selects the path closest to the NUMA node of the current CPU for
I/O distribution. This policy maintains the nearest paths to each NUMA node
based on network interface connections.

When to use the NUMA policy:
  1. Multi-core Systems: Optimizes memory access in multi-core and
     multi-processor systems, especially under NUMA architecture.
  2. High Affinity Workloads: Binds I/O processing to the CPU to reduce
     communication and data transfer delays across nodes.


Round-Robin
-----------

The round-robin policy distributes I/O requests evenly across all paths to
enhance throughput and resource utilization. Each I/O operation is sent to the
next path in sequence.

When to use the round-robin policy:
  1. Balanced Workloads: Effective for balanced and predictable workloads with
     similar I/O size and type.
  2. Homogeneous Path Performance: Utilizes all paths efficiently when
     performance characteristics (e.g., latency, bandwidth) are similar.


Queue-Depth
-----------

The queue-depth policy manages I/O requests based on the current queue depth
of each path, selecting the path with the least number of in-flight I/Os.

When to use the queue-depth policy:
  1. High load with small I/Os: Effectively balances load across paths when
     the load is high, and I/O operations consist of small, relatively
     fixed-sized requests.
