#!/bin/bash

# Setup script to download JUCE framework for the plugin

echo "Setting up JUCE framework..."

# Check if JUCE directory exists
if [ -d "JUCE" ]; then
    echo "JUCE directory already exists. Skipping download."
    exit 0
fi

# Download JUCE using git
echo "Downloading JUCE from GitHub..."
git clone --depth 1 --branch 8.0.0 https://github.com/juce-framework/JUCE.git

if [ $? -eq 0 ]; then
    echo "JUCE downloaded successfully!"
else
    echo "Error: Failed to download JUCE. Please check your internet connection."
    exit 1
fi

echo "Setup complete!"
