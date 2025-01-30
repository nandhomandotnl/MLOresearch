import os

# Lijst van nWifi-waarden
nWifi_values = [2, 5, 10, 15, 20, 25, 30, 35]

# Loop door elke waarde
for n in nWifi_values:
    print(f"Running simulation with nWifi={n}")
    # Voer het commando uit
    os.system(f'./ns3 run "scratch/testc1.cc --nWifi={n}"')

