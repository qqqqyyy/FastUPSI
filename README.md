# FastUPSI


### Environment Test
```bash
docker build --platform=linux/amd64 -t fast_upsi_env .

# run docker, sync with upsi folder outside docker
docker run --platform linux/amd64 -it --rm -v "$(pwd)/upsi":/home/fast-upsi/upsi fast_upsi_env

# (in docker container)
bazel build //upsi/original:all

# setup
./bazel-bin/upsi/original/setup --days=1 --daily_size=64 --start_size=262080

# run party1 first
./bazel-bin/upsi/original/run --party=1 --days=1 & ./bazel-bin/upsi/original/run --party=0 --days=1
```