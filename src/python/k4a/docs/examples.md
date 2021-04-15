# Example Programs Using Azure Kinect Python API(K4A)

The examples/ folder contains Python programs to help guide users in using the
Azure Kinect Python API effectively.

## Example Prerequisites

The following are required in order to run the Python K4A examples.

1. Python (>= 3.6)

2. An internet connection to download required Python packages.

3. The Python K4A package has been installed. To install this package in your
   python environment, get the K4A wheel file (.whl) and "pip install <*.whl>".
   The wheel file can be created from source using the instructions in 
   [building](./building.md).

4. An attached Azure Kinect device.

Each example has additional python package requirements that are specified below.

## Running Examples

### the_basics.py

A simple program to showcase the basic functions in the Azure Kinect API.

Additional Prerequisites: None

To run, open a command terminal and type:  
`python the_basics.py`  

The expected output is a print out on the terminal of the device, capture, and image properties.

### image_transformations.py

A simple program to showcase the image transformation functions in the Azure Kinect API.

Additional Prerequisites:  
- Matplotlib installed via pip:  `pip install matplotlib`  
- Numpy installed via pip:       `pip install numpy==1.18.5`  
- tkinter installed via pip:     `pip install tk`  
    
>**Note:** numpy version 1.19.5 currently has a bug for Windows, so do not use it as of 2020/12/28.

In Linux, another way to install tkinter is: `sudo apt install python3-tk`  

To run, open a command terminal and type:  
`python image_transformations.py`  


The program transforms a color image into the depth camera space and displays the images in a figure.
Close the figure to proceed.  


The program then transforms a depth image into the color camera space and displays the images in a figure.
Close the figure to proceed.  


The program then transforms a depth image and an IR image into the color camera space and displays them in a figure.
Close the figure to proceed and the program exits.  

### simple_viewer.py

A simple program to continuously capture images from an Azure Kinect device and display the images.

Additional Prerequisites:  
- Matplotlib installed via pip:  `pip install matplotlib`  
    
To run, open a command terminal and type:  
`python simple_viewer.py`  

The program creates a figure and continuously displays the captured images on the figure.  
Close the figure to end the program.  
