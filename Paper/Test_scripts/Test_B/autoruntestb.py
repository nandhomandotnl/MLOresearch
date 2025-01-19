import subprocess
import os
import csv
import pyshark

# Functie om de simulatie uit te voeren en de resultaten te verwerken
def run_simulation(nWifi):
    # Run de simulatie met het opgegeven nWifi
    cmd = f"./ns3 run 'scratch/testb_test.cc --simtime=2 --nWifi={nWifi}'"
    print(f"Running simulation with nWifi={nWifi}...")
    subprocess.run(cmd, shell=True)

    # Bestanden voor de pcap-gegevens
    pcap_files = [f'cap-0-0-{i}.pcap' for i in range(3)]

    # Analyseren van de pcap-bestanden
    packets_per_link = {}
    total_packets = 0

    for i, pcap_file in enumerate(pcap_files):
        link_number = f"Link {i + 1}"
        packet_count = count_packets(pcap_file)
        packets_per_link[link_number] = packet_count
        total_packets += packet_count

    # Bereken het percentage per link
    link_percentages = {}
    for link, packet_count in packets_per_link.items():
        if total_packets > 0:
            percentage = (packet_count / total_packets) * 100
            link_percentages[link] = percentage

    return link_percentages

# Functie om het aantal pakketten te tellen in een pcap-bestand
def count_packets(pcap_file):
    try:
        cap = pyshark.FileCapture(pcap_file)
        packet_count = len([pkt for pkt in cap if 'IP' in pkt])  # Tell alleen IP-pakketten
        cap.close()
        return packet_count
    except Exception as e:
        print(f"Fout bij het verwerken van {pcap_file}: {e}")
        return 0

# Functie om resultaten naar CSV-bestand te schrijven
def write_to_csv(results):
    # Bepaal de bestandsnaam en schrijf de header als het bestand nog niet bestaat
    output_file = "link_distribution.csv"
    file_exists = os.path.exists(output_file)

    with open(output_file, mode='a', newline='') as file:
        writer = csv.writer(file)
        
        # Als het bestand nieuw is, schrijf dan de header
        if not file_exists:
            header = ["Link"] + [str(nWifi) for nWifi in results.keys()]
            writer.writerow(header)

        # Schrijf de resultaten naar het CSV-bestand
        for link in ["Link 1", "Link 2", "Link 3"]:
            row = [link] + [f"{results[nWifi].get(link, 0.0):.2f}" for nWifi in results]
            writer.writerow(row)

# Lijst van nWifi waarden om simulaties uit te voeren
nWifi_values = [2, 5, 10, 15, 20]

# Dictionary om de resultaten van alle simulaties op te slaan
all_results = {}

# Voer de simulaties uit voor elke waarde van nWifi en sla de resultaten op
for nWifi in nWifi_values:
    link_percentages = run_simulation(nWifi)
    all_results[nWifi] = link_percentages

# Schrijf alle resultaten naar het CSV-bestand
write_to_csv(all_results)

