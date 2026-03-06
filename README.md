<div align="center">

# Fast UPSI

**Updatable Private Set Intersection from Symmetric-Key Techniques**

*Junxin Liu, Peihan Miao, Mike Rosulek, Xinyi Shi, and Jifeng Wang*


[**Overview**](#overview) | [**Building**](#building-the-project) | [**Experiments**](#running-the-experiments)
| [**Contact**](#author-contact-information) | [**License**](#license)

</div>

---

## Overview

**Fast UPSI** implements *Updatable Private Set Intersection (UPSI)* protocols as detailed in our [Eurocrypt 2026 paper](https://eprint.iacr.org/2026/438). Private Set Intersection (PSI) enables two mutually distrusting parties, each holding a private set of elements, to compute the intersection of their sets without disclosing any additional information. Our UPSI project addresses several key limitations of existing UPSI protocols:

- **Symmetric-Key UPSI:** In existing UPSI protocols, the number of public-key
operations scales with the number of items. Our UPSI project largely avoids public-key operations to achieve orders-of-magnitude improvements over prior work.

- **Security Against Adaptive Inputs:** UPSI is a reactive functionality where inputs and outputs happen over many rounds of interaction. In the standard MPC security model, an adversary may choose inputs to subsequent rounds *adaptively*, based on its view of the protocol from previous rounds. Prior UPSI protocols use a strictly weaker security definition, and we identify several existing protocols
which were proven secure in the selective setting, but are insecure against a semi-honest adversary that adaptively chooses inputs (see attacks in Section 2 of the [paper](https://eprint.iacr.org/2026/438)). Our UPSI project contains several protocols that are secure in the presence of adaptive inputs.

> [!WARNING]
> This repository is a research prototype written to demonstrate our UPSI protocol's performance and to showcase its capabilities. It is **NOT** intended to be considered as "production ready" and should only be used for experimental or research & development purposes.

### Protocol Descriptions

We implement three UPSI protocols, each using a different *Affine Set Encoding (ASE)* construction with different trade-offs in functionality, security, and efficiency. See Section 6.1 of the [paper](https://eprint.iacr.org/2026/438) for details.

| Protocol | ASE Construction | Addition | Deletion | Security Against Adaptive Inputs |
|:--------:|:----------------:|:--------:|:--------:|:--------------------------------:|
| `tree` | Path-ORAM-based | ✓ | ✓ | ✗ |
| `okvs` | Adaptive + RB-OKVS | ✓ | ✗ | ✓ |
| `cuckoo` | Adaptive + Cuckoo Hashing | ✓ | ✓ | ✓ |


**Achieve Security Against Adaptive Inputs:** We propose an adaptive construction that converts non-updatable ASEs into updatable, unbounded size, adaptively correct/secure ASEs (see Section 3.5 of the [paper](https://eprint.iacr.org/2026/438)). Our `okvs` and `cuckoo` protocols apply the adaptive construction to achieve security against such adaptive inputs, while the `tree` protocol is secure against non-adaptive (statically chosen) inputs.

## Building the Project

The project is built on top of [libOTe](https://github.com/osu-crypto/libOTe/).

### Container Setup with Docker

The repository has been containerized using Docker. To pull the appropriate container:
```bash
docker pull --platform=linux/amd64 ghcr.io/qqqqyyy/fastupsi:latest
```

To run the container:
```bash
docker run -it --platform=linux/amd64 ghcr.io/qqqqyyy/fastupsi:latest
```

> [!WARNING]
> If you run into an error `RTNETLINK answers: Operation not permitted`, add `--cap-add=NET_ADMIN` to the `docker run` command.


### Building Locally

> [!IMPORTANT]
> This project should be built locally on Ubuntu 24.04 (x86_64/amd64).
> Other environments (e.g., macOS / Apple Silicon) may work by using similar commands provided in `build.sh`, but are **not guaranteed**.

Clone the repository and run the build script:

```bash
git clone https://github.com/qqqqyyy/FastUPSI.git
cd FastUPSI
chmod +x build.sh
./build.sh
```

## Running the Experiments

From the repository root, cd into the `build` directory:
```bash
cd build
```

Run the experiments by using `setup` and `main` under `frontend` folder.

### Generate Datasets

Before running experiments, use the `setup` binary to generate both parties' private sets:

```bash
# Make sure you are in the build directory
./frontend/setup -start_size <initial_set_size> -add_size <additions_per_day> -del_size <deletions_per_day> -days <num_days>
```

**Options:**
- `-start_size`: Initial set size (for each party)
- `-add_size`: Number of elements added per day (for each party)
- `-del_size`: Number of elements deleted per day (for each party)
- `-days`: Number of update days to simulate (default: 8)

> [!WARNING]
> To run experiments with addition-only updates, disable deletions by setting `-del_size` to `0`.

#### Examples
```bash
# Generate datasets with 1024 initial elements, 128 additions and 16 deletions per day, over 8 days
./frontend/setup -start_size 1024 -add_size 128 -del_size 16 -days 8
# Generate datasets with 0 initial elements, 1024 additions per day and no deletions, over 128 days
./frontend/setup -start_size 0 -add_size 1024 -del_size 0 -days 128
```

### Run UPSI

Run each party using `main` binary:

```bash
# Make sure you are in the build directory
./frontend/main -party <0|1> -prot <tree|okvs|cuckoo> -days <num> [-del]
```

**Options:**
- `-party <0|1>`: Party ID (must run both party 0 and party 1)
- `-prot <tree|okvs|cuckoo>`: Protocol to be used in the experiment.
- `-days <num>`: Number of update days (default: 8)
- `-del`: Enable deletion. If deletions are not needed, omit the `-del` flag.

> [!WARNING]
> The `okvs` protocol does not support deletion; therefore, it can only be evaluated on addition-only datasets.

**Network Settings:**

To simplify the setup of network conditions for experiments, the [network_setup.sh](network_setup.sh) script is provided in the base directory. This script automates the configuration of network bandwidth and latency, simulating both LAN and WAN environments.

The paper explains the network conditions used for experiments, which follow the same settings as previous works:
- **LAN Connection:**
  - **RTT (Round Trip Time):** 0.2 ms
  - **Bandwidth:** 1 Gbps
  - Run `main` with `-LAN` to enable the LAN setting
- **WAN Connection:**
  - **RTT (Round Trip Time):** 80 ms
  - **Bandwidth Options:** 200 Mbps, 50 Mbps, and 5 Mbps
  - Run `main` with `-WAN <200|50|5>` to use WAN network settings with specified bandwidth (200, 50, or 5 Mbps)

To set up a specific network setting that is not used in the paper, use [network_setup.sh](network_setup.sh) script:
1. **Enable a Specific Network Setting:** 
```bash
./network_setup.sh on <latency> <bandwidth>
```
Examples: 
  - 0.2ms RTT (latency = 0.1ms), 1Gbps
  ```bash
  ./network_setup.sh on 0.1 1000
  ```
  - 80ms RTT (latency = 40ms), 200Mbps
  ```bash
  ./network_setup.sh on 40 200
  ```
2. **Disable Network Emulation:** After completing the experiments under the specified network condition, disable the network emulation by running:
  ```bash
  ./network_setup.sh off
  ```

#### Example Commands

Make sure you are in the `build` directory, and have generated the dateset using the `setup` binary.
- tree protocol, 8 days, deletion
```bash
./frontend/main -party 0 -prot tree -del
```
```bash
./frontend/main -party 1 -prot tree -del
```
- okvs(adaptive) protocol, 128 days, LAN
```bash
./frontend/main -party 0 -prot okvs -days 128 -LAN 
```
```bash
./frontend/main -party 1 -prot okvs -days 128 -LAN
```
- cuckoo hashing(adaptive) protocol, 8 days, deletion, WAN 200Mbps
```bash
./frontend/main -party 0 -prot cuckoo -del -WAN 200
```
```bash
./frontend/main -party 1 -prot cuckoo -del -WAN 200
```

### Example Workflow for Experiments

```bash
# Make sure you are in the build directory
./frontend/setup -start_size 1024 -add_size 128 -del_size 16 -days 8
# cuckoo hashing(adaptive) protocol, 8 days, deletion, WAN 200Mbps
./frontend/main -party 0 -prot cuckoo -del -WAN 200 & ./frontend/main -party 1 -prot cuckoo -del -WAN 200
```


## Author Contact Information

Feel free to reach out to the authors for further questions:

| Name                  | Affiliation       | Contact                              |
|-----------------------|-------------------|--------------------------------------|
| **Xinyi Shi**         | Brown University  | `xinyi_shi [at] brown [dot] edu`    |
| **Jifeng Wang**  | Brown University  | `jifeng_wang [at] brown [dot] edu` |

## License

This project is licensed under the [Apache License 2.0](LICENSE).
