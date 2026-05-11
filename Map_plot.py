#!/usr/bin/python3
import folium
import webbrowser

in_file = "out.txt"
mapfile = in_file.split('.')[-2] + ".html"

latitudes = []
longitudes = []

with open(in_file, 'r') as f:
    for line in f:
        if line.startswith("#"):
            continue
        fields = line.split('|')
        # Assuming format: Id=... | lat | lon | Dist=...
        lat = float(fields[1].strip())
        lon = float(fields[2].strip())
        latitudes.append(lat)
        longitudes.append(lon)

# Create a map centered on the first point
m = folium.Map(location=[latitudes[0], longitudes[0]], zoom_start=14)

# Add markers and polyline
folium.PolyLine(list(zip(latitudes, longitudes)), color='blue', weight=2.5).add_to(m)
for lat, lon in zip(latitudes, longitudes):
    folium.CircleMarker([lat, lon], radius=3, color='red', fill=True).add_to(m)

m.save(mapfile)
webbrowser.open(mapfile)