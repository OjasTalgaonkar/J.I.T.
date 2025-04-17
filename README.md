# J.I.T. - Justified Internet Tracker

**J.I.T.** (Justified Internet Tracker) is a work-in-progress tool aimed at simplifying network packet analysis for beginners. The goal is to provide an easy-to-use interface that helps users understand network traffic and packet data. Currently, the project is in its early stages, with core features being developed and refined.

## Features:

- Simplified packet analysis for newcomers
- User-friendly interface for easy understanding of network traffic

## Installation:

1. Clone the repository:
   ```bash
   git clone https://github.com/OjasTalgaonkar/J.I.T.
   ```
2. [Install Npcap](#npcap-installation)

3. Run this command in the terminal
   ```bash
   build/make.sh
   ```
   to compile the executables and run them.

## Contributing

Feel free to fork the repository and contribute. Contributions are welcome, and suggestions for improving the tool are highly encouraged.

<details id="npcap-installation"> <summary><strong>Npcap Installation Guide (Without SDK)</strong></summary>
Npcap is required for packet capture functionality. Follow these steps to install Npcap without the SDK:

Download Npcap from the official website: Npcap Download.
Run the installer and ensure that the option for "Install Npcap in WinPcap API-compatible Mode" is checked.
During installation, uncheck the "Install Npcap SDK" option.
Complete the installation process.
Once installed, Npcap will be ready for use with J.I.T. for network packet analysis.

</details>
