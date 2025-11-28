# AeroSecure SyncNet – Encrypted Dual-Node Communication for Intelligent Data Transfer

🔐 A dual-node ESP32 secure communication system that transmits radar-generated data using AES encryption over UDP with both hardware-based and cloud-based data processing.

---

## 🔧 System Implementations

### **1️⃣ Hardware-Based Implementation**
- Radar sensor measures real-time object distance  
- Sender ESP32 encrypts data using AES algorithm  
- Data is transmitted wirelessly over UDP  
- Receiver ESP32 decrypts data  
- System calculates:
  - Minimum, maximum, and average distance  
  - Nearest object direction  
  - Velocity of detected objects  
- Results are displayed on an external LCD module  

➡ Demonstrates secure **embedded communication**, on-chip AES encryption, and standalone real-time processing without internet dependency.

---

### **2️⃣ Cloud-Based Implementation**
- Radar data is transmitted to a cloud analytics environment  
- Data is stored and processed for extended analysis  
- Cloud platform calculates and visualizes:  
  - **Distance between detected objects**  
  - Motion trend analysis  
  - Log-based comparison over time  
- Cloud dashboard can be accessed remotely for monitoring and visualization  

➡ Enables **remote monitoring, scalability, analytics, and persistent storage** beyond hardware-only limitations.

---

## 🚀 Features

- AES-encrypted wireless communication  
- Dual ESP32 node architecture (Sender/Receiver)  
- Real-time secure distance monitoring  
- LCD-based local display output  
- Cloud dashboard for extended analytics  
- CSV-based logged data for offline analysis  
- Object detection metrics:  
  - Min/Max/Avg distance  
  - Direction  
  - Velocity  
  - Distance between objects (cloud)

---

## 🛠 Tech Stack

**Hardware**
- ESP32 development boards  
- LCD Display Module  
- Wi-Fi Communication

**Software**
- Arduino IDE (C/C++)  
- AES Encryption Library  
- UDP communication protocol  
- Google Colab (Cloud data analysis + visualization)  
- CSV data processing tools  

---

## 📂 Project Structure

```text
📦 AeroSecure-SyncNet
 ┣ 📂 src
 ┃ ┣ sender.ino                 # Sender ESP32 firmware
 ┃ ┗ receiver.ino               # Receiver ESP32 firmware
 ┣ 📂 cloud
 ┃ ┗ AeroSecure_CloudAnalysis.ipynb   # Google Colab notebook for cloud processing & visualization
 ┣ 📂 output
 ┃ ┗ output.csv                 # Logged radar data for analysis
 ┣ 📂 documentation
 ┃ ┣ hardware_setup.png         # Optional: images, circuit diagrams
 ┃ ┗ cloud_dashboard.png        # Optional: screenshot of cloud result
 ┗ README.md
