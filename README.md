<div align="center">

# Fast UPSI

**Updatable Private Set Intersection from Symmetric-Key Techniques**

*Junxin Liu, Peihan Miao, Mike Rosulek, Xinyi Shi, and Jifeng Wang*


[**Overview**](#overview) | [**Building**](#building-the-project) | [**Experiments**](#running-the-experiments)
| [**Contact**](#author-contact-information) | [**License**](#license)

</div>

---

## Overview

**Fast UPSI** implements *Updatable Private Set Intersection (UPSI)* protocols of ``Updatable Private Set Intersection from Symmetric-Key Techniques'' paper. Private Set Intersection (PSI) enables two mutually distrusting parties, each holding a private set of elements, to compute the intersection of their sets without disclosing any additional information. 

**Practical Performance:** We have implemented our UPSI protocols and benchmarked them against state-of-the-art PSI and UPSI protocols. By leveraging symmetric-key primitives, our implementation achieves orders-of-magnitude improvements over prior work, particularly when operating within high-bandwidth network environments.

> [!WARNING]
> This repository is a research prototype written to demonstrate our UPSI protocol's performance and to showcase its capabilities. It is **NOT** intended to be considered as "production ready" and should only be used for experimental or research & development purposes.

### Protocol Descriptions

We implement three UPSI protocols, each using different Affine Set Encoding (ASE) constructions with distinct trade-offs in efficiency, security, and functionality. See Section 6.1 of the paper for details.

| Protocol | ASE Construction | Addition | Deletion | Security Against Adaptive Inputs|
|----------|------------------|----------|----------|-------------------|
| `tree` | Path-ORAM-based | ✓ | ✓ | ✗ |
| `okvs` | Adaptive + RB-OKVS | ✓ | ✗ | ✓ |
| `cuckoo` | Adaptive + Cuckoo Hashing | ✓ | ✓ | ✓ |


**Security Against Adaptive Inputs:** UPSI is a reactive functionality where inputs and outputs occur over many rounds of interaction. In the standard MPC security model, an adversary may choose inputs to subsequent rounds *adaptively*, based on its view of the protocol from previous rounds. In the paper, we propose an adaptive construction that converts non-updatable ASEs into updatable, unbounded size, adaptively correct/secure ASEs. Our `okvs` and `cuckoo` protocols apply the adaptive construction to achieve security against such adaptive inputs, while the `tree` protocol is secure against non-adaptive (statically chosen) inputs.

## Building the Project

The project is built on top of [libOTe](https://github.com/osu-crypto/libOTe/), which provides efficient vector oblivious linear evaluation (VOLE) and Puncturable Pseudorandom Function (PPRF) primitives.

### Building Locally

Clone the repository and run the build script:

```bash
git clone https://github.com/qqqqyyy/FastUPSI.git
cd FastUPSI
chmod +x build.sh
./build.sh
```

### Building with Docker

Build and run the Docker container:

```bash
docker build -t fastupsi .
docker run -it fastupsi
```

## Running the Experiments

Run the experiments by using `setup` and `main` under `frontend` folder.

### Generate Datasets

Before running experiments, use the `setup` binary to generate two parties' private sets:

```bash
./frontend/setup -start_size <initial_set_size> -add_size <additions_per_day> -del_size <deletions_per_day> -days <num_days>
```

**Parameters:**
- `-start_size`: Initial set size
- `-add_size`: Number of elements added per day
- `-del_size`: Number of elements deleted per day (set to `0` if no deletions)
- `-days`: Number of update days to simulate (default: 8)

#### Examples
```bash
# Generate datasets with 64 initial elements, 32 additions and 16 deletions per day, over 128 days
./frontend/setup -start_size 64 -add_size 32 -del_size 16 -days 128
```

### Run UPSI

Run each party using `main` binary under `frontend` folder.

#### Options

- `-party <0|1>`: Party ID (must run both party 0 and party 1)
- `-prot <tree|okvs|hash>`: Protocol to be used in the experiment.
- `-days <num>`: Number of update days (default: 8)
- `-del`: Enable deletion

#### Network Settings

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

#### Examples

```bash
# tree, 8 days, deletion
./frontend/main -party 1 -prot tree -del & ./frontend/main -party 0 -prot tree -del
# okvs(adaptive), 128 days, LAN
./frontend/main -party 1 -prot okvs -days 128 -LAN & ./frontend/main -party 0 -prot okvs -days 128 -LAN
# cuckoo hashing(adaptive), 8 days, deletion, WAN 200Mbps
./frontend/main -party 1 -prot cuckoo -del -WAN 200 & ./frontend/main -party 0 -prot cuckoo -del -WAN 200
```


## Author Contact Information

Feel free to reach out to the authors for further questions:

| Name                  | Affiliation       | Contact                              |
|-----------------------|-------------------|--------------------------------------|
| **Xinyi Shi**         | Brown University  | `xinyi_shi [at] brown [dot] edu`    |
| **Jifeng Wang**  | Brown University  | `jifeng_wang [at] brown [dot] edu` |

## License

This project is licensed under the [Apache License 2.0](LICENSE).
