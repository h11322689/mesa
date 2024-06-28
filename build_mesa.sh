#!/bin/bash

start_time=$(date +%s)  # Record start time

# git tag | grep 23.3
# git checkout mesa-23.3.6 -b mesa-23.3.6
# build-dep需要将软件源中的deb-src取消注释
# sudo apt build-dep mesa

meson build
ninja -C build

end_time=$(date +%s)                            # Record end time
duration=$((end_time - start_time))             # Calculate duration in seconds

echo "Build duration: $duration seconds"        # Print compilation duration

