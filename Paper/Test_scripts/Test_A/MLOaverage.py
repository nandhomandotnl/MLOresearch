import xml.etree.ElementTree as ET
import csv
import os
import re


def parse_float(value):
    """Converteer een string naar een float, waarbij eenheden zoals 'ns' worden verwijderd."""
    try:
        return float(value.replace("ns", "").strip())
    except ValueError:
        return 0.0


def calculate_averages(xml_file):
    """Bereken gemiddelde statistieken voor een gegeven XML-bestand."""
    try:
        tree = ET.parse(xml_file)
        root = tree.getroot()
        flow_stats = root.find("FlowStats")
        if flow_stats is None:
            return None

        total_tx_packets = 0
        total_rx_packets = 0
        total_lost_packets = 0
        total_tx_bytes = 0
        total_rx_bytes = 0
        total_delay_sum = 0
        total_jitter_sum = 0
        total_throughput = 0
        flow_count = 0

        for flow in flow_stats.findall("Flow"):
            tx_packets = int(flow.get("txPackets", 0))
            rx_packets = int(flow.get("rxPackets", 0))
            lost_packets = int(flow.get("lostPackets", 0))
            tx_bytes = int(flow.get("txBytes", 0))
            rx_bytes = int(flow.get("rxBytes", 0))
            delay_sum = parse_float(flow.get("delaySum", "0"))
            jitter_sum = parse_float(flow.get("jitterSum", "0"))
            time_first_rx = parse_float(flow.get("timeFirstRxPacket", "0"))
            time_last_rx = parse_float(flow.get("timeLastRxPacket", "0"))

            duration = time_last_rx - time_first_rx
            throughput = (rx_bytes * 8) / duration / 1e6 if duration > 0 else 0

            total_tx_packets += tx_packets
            total_rx_packets += rx_packets
            total_lost_packets += lost_packets
            total_tx_bytes += tx_bytes
            total_rx_bytes += rx_bytes
            total_delay_sum += delay_sum
            total_jitter_sum += jitter_sum
            total_throughput += throughput
            flow_count += 1

        if flow_count > 0:
            return {
                "file": os.path.basename(xml_file),
                "avg_tx_packets": total_tx_packets / flow_count,
                "avg_rx_packets": total_rx_packets / flow_count,
                "avg_lost_packets": total_lost_packets / flow_count,
                "avg_tx_bytes": total_tx_bytes / flow_count,
                "avg_rx_bytes": total_rx_bytes / flow_count,
                "avg_delay_sum": total_delay_sum / flow_count,
                "avg_jitter_sum": total_jitter_sum / flow_count,
                "avg_throughput": total_throughput / flow_count,
            }
        return None
    except Exception as e:
        print(f"Fout bij verwerken van {xml_file}: {e}")
        return None


def process_all_files(directory, output_csv):
    """Verwerk alle XML-bestanden in een directory en sla de gemiddelden op in een CSV-bestand."""
    files = [
        os.path.join(directory, f)
        for f in os.listdir(directory)
        if re.match(r"flowmon-data-sta-mloinf\d+\.xml", f)
    ]

    if not files:
        print(f"Geen bestanden gevonden in {directory} met het patroon.")
        return

    averages = []
    for file in files:
        avg_data = calculate_averages(file)
        if avg_data:
            averages.append(avg_data)

    # Schrijf de resultaten naar een CSV-bestand
    with open(output_csv, mode="w", newline="") as csv_file:
        writer = csv.DictWriter(
            csv_file,
            fieldnames=[
                "file",
                "avg_tx_packets",
                "avg_rx_packets",
                "avg_lost_packets",
                "avg_tx_bytes",
                "avg_rx_bytes",
                "avg_delay_sum",
                "avg_jitter_sum",
                "avg_throughput",
            ],
        )
        writer.writeheader()
        writer.writerows(averages)

    print(f"Gemiddelde statistieken opgeslagen in: {output_csv}")


if __name__ == "__main__":
    directory = "/home/admin1/ns-3.40/MLO"
    output_csv = "averages.csv"
    process_all_files(directory, output_csv)

