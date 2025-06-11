# FastUPSI


### Environment Test
```bash
docker build --platform=linux/amd64 -t fast_upsi_env .

# run docker, sync with upsi folder outside docker
docker run --platform linux/amd64 -it --rm -v "$(pwd)/upsi":/home/fast-upsi/upsi fast_upsi_env

# (in docker container)
bazel build //upsi/tree:all

# test (run party1 first)
./bazel-bin/upsi/tree/run --party=1 --days=1 --func=PSI & ./bazel-bin/upsi/tree/run --party=0 --days=1 --func=PSI
```