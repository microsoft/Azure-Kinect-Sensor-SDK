// Content taken from https://raw.githubusercontent.com/Microsoft/busiotools/d0daabad2f981f6dd83ab931fb97fca9b8ca86ef/hmdvalidationkit/managed/HmdKit.cs

namespace HmdAutomation
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.IO.Ports;
    using System.Threading;

    /// <summary>
    /// This class provides managed access to the functionality of the HMD Validation Kit.
    /// </summary>
    public class HmdKit : IDisposable
    {
        #region Private Members
        /// <summary>
        /// The default baud rate for the HMD Kit's Serial over USB communication is 9600 baud.
        /// </summary>
        private const int HmdKitBaudRate = 9600;

        /// <summary>
        /// This constant controls the number of retries for sending a command.
        /// </summary>
        private const int RetryCount = 3;

        /// <summary>
        /// This is the string hardcoded in the HMD Validation Kit for the version. This constant should match the expected version from the HMD Validation Kit.
        /// </summary>
        private const string HmdKitVersion = "01";

        /// <summary>
        /// This specifies the type of shield detected by the HMD Validation Kit. The current revision should always detect shield type "8".
        /// </summary>
        private const string HmdKitShieldType = "08";

        /// <summary>
        /// This is the default response time that should provide enough time for a simple command to complete. Use this for commands that don't involve waiting for something to happen on the HMD Validation Kit like taking audio samples or waiting for sensor data.
        /// </summary>
        private const int ResponseTimeDefault = 500;

        /// <summary>
        /// This time should provide enough window for the TCS color sensor's "integration time" to elapse and for the HMD Validation Kit to return a value. See the TCS34725 documentation for more information.
        /// </summary>
        private const int ResponseTimeColorSensor = 2000;

        /// <summary>
        /// This is a non-constant value that should provide enough time for a "GetVolume*" command to run for the specified number of samples. It will change every time the SetVolumeSampleCount method is used.
        /// </summary>
        private int responseTimeGetVolume = 1000;

        /// <summary>
        /// This is the internal instance of the serial port attached to the HMD Validation Kit.
        /// </summary>
        private SerialPort serialPort;

        /// <summary>
        /// This dictionary translates the friendly names of the commands to their real command strings that will be sent over the cable to the HMD Validation Kit.
        /// </summary>
        private Dictionary<string, string> hmdKitCommand = new Dictionary<string, string>();

        /// <summary>
        /// This lock is set when the HMD Validation Kit's serial port is being accessed.
        /// </summary>
        private object hmdKitLock = new object();

        /// <summary>
        /// This private member tracks the enumeration state of the HMD Validation Kit.
        /// </summary>
        private bool isPresent;
        #endregion

        #region Constructors
        /// <summary>
        /// Initializes a new instance of the <see cref="HmdKit"/> class. This maps all of the appropriate friendly names to their serial commands along with initializing the enumeration state to false.
        /// </summary>
        public HmdKit()
        {
            // Map the HmdKit commands
            this.hmdKitCommand.Add("GetVersion", "version");
            this.hmdKitCommand.Add("SetPort", "port");
            this.hmdKitCommand.Add("GetPort", "port");
            this.hmdKitCommand.Add("GetHdmiPort", "gethdmiport");
            this.hmdKitCommand.Add("SetHdmiPort", "sethdmiport");
            this.hmdKitCommand.Add("GetDisplayBrightness", "brightness");
            this.hmdKitCommand.Add("GetDisplayColor", "color");
            this.hmdKitCommand.Add("SetPresence", "presence");
            this.hmdKitCommand.Add("GetVolumeRms", "getvolumerms");
            this.hmdKitCommand.Add("GetVolumeAvg", "getvolumeavg");
            this.hmdKitCommand.Add("GetVolumePeak", "getvolumepeak");
            this.hmdKitCommand.Add("SetVolumeSampleCount", "svsc");
            this.hmdKitCommand.Add("SetServoAngle", "setangle");
            this.hmdKitCommand.Add("SetHmd", "sethmd");
            this.hmdKitCommand.Add("Volts", "volts");
            this.hmdKitCommand.Add("Amps", "amps");
            this.hmdKitCommand.Add("SuperSpeed", "superspeed");
            this.hmdKitCommand.Add("Delay", "delay");
            this.hmdKitCommand.Add("Timeout", "timeout");
            this.isPresent = false;
        }
        #endregion

        #region Properties
        /// <summary>
        /// Gets a value indicating whether the HMD Validation Kit is enumerated.
        /// </summary>
        public bool IsPresent
        {
            get
            {
                return this.isPresent;
            }
        }
        #endregion

        #region Methods
        /// <summary>
        /// Implementation of the Dispose interface to dispose of the managed resources used by the HmdKit class.
        /// </summary>
        public void Dispose()
        {
            this.Dispose(true);
            GC.SuppressFinalize(this);
        }

        /// <summary>
        /// Checks all open serial ports for HMD Exercisers
        /// </summary>
        /// <returns>
        /// Returns true if a valid HMD Kit is detected, otherwise returns false.
        /// </returns>
        public bool FindHmdKitDevice()
        {
            lock (this.hmdKitLock)
            {
                if (this.serialPort != null)
                {
                    if (this.serialPort.IsOpen)
                    {
                        string response = string.Empty;
                        this.SendCommand("GetVersion", ResponseTimeDefault, null, out response);
                        if (response == (HmdKitVersion + HmdKitShieldType))
                        {
                            this.isPresent = true;
                            return true;
                        }
                        else
                        {
                            this.serialPort.Close();
                        }
                    }
                }

                string[] ports = SerialPort.GetPortNames();
                foreach (string port in ports)
                {
                    SerialPort sp = new SerialPort(port, HmdKitBaudRate, Parity.None, 8, StopBits.One);
                    sp.Handshake = Handshake.None;
                    sp.DtrEnable = true;
                    sp.ReadTimeout = 500;
                    try
                    {
                        sp.Open();
                    }
                    catch (UnauthorizedAccessException)
                    {
                        // Serial port is already open or access is denied
                        continue;
                    }
                    catch (InvalidOperationException)
                    {
                        // The specified port on the current instance of the SerialPort is already open
                        continue;
                    }
                    catch (IOException)
                    {
                        continue;
                    }

                    // Wait for the device to initialize
                    Thread.Sleep(2000);

                    sp.NewLine = "\r\n";
                    this.serialPort = sp;
                    string response = string.Empty;
                    this.serialPort.WriteLine("version");
                    try
                    {
                        response = this.serialPort.ReadLine();
                    }
                    catch (TimeoutException)
                    {
                        sp.Dispose();
                        continue;
                    }

                    if (response == (HmdKitVersion + HmdKitShieldType))
                    {
                        this.isPresent = true;
                        return true;
                    }
                    else
                    {
                        sp.Dispose();
                    }
                }

                this.serialPort = null;
                this.isPresent = false;
                return false;
            }
        }

        /// <summary>
        /// Sends a command to the HMD Exerciser. The command should return within responseTime milliseconds.
        /// </summary>
        /// <param name="command">
        /// The command to send to the HMD Kit. The command should be a part of the hmdKitCommand Dictionary.
        /// </param>
        /// <param name="responseTime">
        /// The time that the SendCommand method should wait for a response. This can be longer for commands that will take more time.
        /// </param>
        /// <param name="parameter">
        /// An parameter for commands that set USB ports, HDMI ports, etc. This will be appended to the command and sent to the HMD Kit.
        /// </param>
        /// <param name="output">
        /// Returns with the response for commands that need to get information from the HMD Kit</param>
        public void SendCommand(string command, int responseTime, string parameter, out string output)
        {
            output = null;
            lock (this.hmdKitLock)
            {
                string serialCommand;
                if (this.serialPort == null)
                {
                    throw new InvalidOperationException("The device is not connected.");
                }

                if (!this.serialPort.IsOpen)
                {
                    throw new InvalidOperationException("The device is not connected.");
                }

                for (int i = 0; i < RetryCount; i++)
                {
                    this.serialPort.ReadTimeout = responseTime;
                    this.serialPort.ReadExisting();

                    if (this.hmdKitCommand.TryGetValue(command, out serialCommand))
                    {
                        if (parameter != null)
                        {
                            serialCommand += " " + parameter;
                        }

                        this.serialPort.WriteLine(serialCommand);
                        try
                        {
                            string response = this.serialPort.ReadLine();
                            response = response.TrimStart(' ');
                            if (!string.IsNullOrEmpty(response))
                            {
                                if (response.Contains(serialCommand))
                                {
                                    response = response.Replace(serialCommand, string.Empty);
                                }

                                if (response.Length > 0)
                                {
                                    output = response;
                                    return;
                                }
                                else
                                {
                                    throw new InvalidOperationException("The device did not return the correct response. Check the status of the device.");
                                }
                            }
                        }
                        catch (TimeoutException)
                        {
                            // Retry
                        }
                    }
                }

                throw new InvalidOperationException("The device did not return the correct response. Check the status of the device.");
            }
        }

        /// <summary>
        /// Sends a command to the HMD Exerciser. The command should return within responseTime milliseconds. This overload doesn't have an output and the parameter is optional.
        /// </summary>
        /// <param name="command">
        /// The command to send to the HMD Kit. The command should be a part of the hmdKitCommand Dictionary.
        /// </param>
        /// <param name="responseTime">
        /// The time that the SendCommand method should wait for a response. This can be longer for commands that will take more time.
        /// </param>
        /// <param name="parameter">
        /// An optional parameter for commands that set USB ports, HDMI ports, etc. This will be appended to the command and sent to the HMD Kit.
        /// </param>
        public void SendCommand(string command, int responseTime, string parameter = null)
        {
            lock (this.hmdKitLock)
            {
                string serialCommand;
                if (this.serialPort == null)
                {
                    throw new InvalidOperationException("The device is not connected.");
                }

                if (!this.serialPort.IsOpen)
                {
                    throw new InvalidOperationException("The device is not connected.");
                }

                for (int i = 0; i < RetryCount; i++)
                {
                    this.serialPort.ReadTimeout = responseTime;
                    this.serialPort.ReadExisting();

                    if (this.hmdKitCommand.TryGetValue(command, out serialCommand))
                    {
                        if (parameter != null)
                        {
                            serialCommand += " " + parameter;
                        }

                        this.serialPort.WriteLine(serialCommand);
                        try
                        {
                            string response = this.serialPort.ReadLine();
                            response = response.TrimStart(' ');
                            if (!string.IsNullOrEmpty(response))
                            {
                                if (response == serialCommand || response == parameter.Split(' ')[0])
                                {
                                    return;
                                }
                                else
                                {
                                    throw new InvalidOperationException("The device did not return the correct response. Check the status of the device.");
                                }
                            }
                        }
                        catch (TimeoutException)
                        {
                            // Retry
                        }
                    }
                }
            }

            throw new InvalidOperationException("The device was not able to be reached. Check the status of the device.");
        }

        /// <summary>
        /// Sets the USB port that is connected to J1.
        /// </summary>
        /// <param name="port">
        /// Ports can be either 0,1,2,3,4 or go by the silkscreen on the PCB: J2,J3,J4, or J6. 
        /// The mapping is J2=1 J3=2 J4=4 J6=3
        /// </param>
        public void SetUsbPort(string port)
        {
            if (string.IsNullOrEmpty(port))
            {
                throw new ArgumentNullException();
            }

            if (port.Length > 2 || port.Length < 1)
            {
                throw new ArgumentOutOfRangeException("Valid ports are J2, J3, J4, J6, or 1, 2, 3, 4");
            }

            port = port.ToLower();
            if (port[0] == 'j')
            {
                char jPort = port[1];

                switch (jPort)
                {
                    case '2':
                        port = "1";
                        break;
                    case '3':
                        port = "2";
                        break;
                    case '4':
                        port = "4";
                        break;
                    case '6':
                        port = "3";
                        break;
                    default:
                        throw new ArgumentOutOfRangeException("Valid ports are J2, J3, J4, J6, or 1, 2, 3, 4");
                }
            }

            try
            {
                this.SendCommand("SetPort", ResponseTimeDefault, port);
            }
            catch
            {
                throw;
            }
        }

        /// <summary>
        /// Gets the currently connected USB port on the HMD Exerciser.
        /// </summary>
        /// <returns>
        /// Returns the currently connected USB port. '0' designates no port is connected.
        /// </returns>
        public string GetUsbPort()
        {
            string port = string.Empty;
            try
            {
                this.SendCommand("GetPort", ResponseTimeDefault, null, out port);
            }
            catch
            {
                throw;
            }

            return port;
        }

        /// <summary>
        /// Sets the currently connected HDMI port.
        /// </summary>
        /// <param name="port">
        /// The port to connect. Valid values are J2/J3 or 1/2. J2 and 1 are equivalent and J3 and 2 are equivalent.
        /// </param>
        public void SetHdmiPort(string port)
        {
            if (string.IsNullOrEmpty(port))
            {
                throw new ArgumentNullException();
            }

            if (port.Length > 2 || port.Length < 1)
            {
                throw new ArgumentOutOfRangeException("Valid ports are J2, J3 or 1, 2");
            }

            port = port.ToLower();
            if (port[0] == 'j')
            {
                char jPort = port[1];

                switch (jPort)
                {
                    case '2':
                        port = "1";
                        break;
                    case '3':
                        port = "2";
                        break;
                    default:
                        throw new ArgumentOutOfRangeException("Valid ports are J2, J3 or 1, 2");
                }
            }

            try
            {
                this.SendCommand("SetHdmiPort", ResponseTimeDefault, port);
            }
            catch
            {
                throw;
            }
        }

        /// <summary>
        /// Gets the index of the currently connected HDMI port from the HMD Exerciser.
        /// </summary>
        /// <returns>
        /// Returns the currently connected HDMI port. '0' means no port is connected. Otherwise, the port will be 2(J2) or 1(J3)
        /// </returns>
        public string GetHdmiPort()
        {
            string port = string.Empty;
            try
            {
                this.SendCommand("GetHdmiPort", ResponseTimeDefault, null, out port);
                return port;
            }
            catch
            {
                throw;
            }
        }

        /// <summary>
        /// Sets the angle of the specified servo in degrees.
        /// </summary>
        /// <param name="servo">
        /// The servo to set the angle on. See the silkscreen on the servo connectors of the HMD Exerciser.
        ///     1 Selects "Servo 1"
        ///     2 Selects "Servo 2"
        /// </param>
        /// <param name="degrees">
        /// The degree position to move to in the servo's sweep. The degrees should be between 45* and 135* for most servos.
        /// </param>
        public void SetServoAngle(string servo, string degrees)
        {
            string parameter = servo + " " + degrees;
            try
            {
                this.SendCommand("SetServoAngle", ResponseTimeDefault, parameter);
            }
            catch
            {
                throw;
            }
        }

        /// <summary>
        /// Sets the presence spoofing LED to either drown out the HMD's presence sensor with light (no user presence) or spoof user presence.
        /// </summary>
        /// <param name="userPresent">
        /// The desired user presence state
        ///     true User presence is spoofed by imitating the IR presence sensor response pattern.
        ///     false User presence is "removed" by flooding the IR presence sensor with IR light, removing the ability for it to see the reflection of the response pattern.
        /// </param>
        public void SetPresence(bool userPresent)
        {
            int parameter = (userPresent == true) ? 1 : 0;
            try
            {
                this.SendCommand("SetPresence", ResponseTimeDefault, parameter.ToString());
            }
            catch
            {
                throw;
            }
        }

        /// <summary>
        /// Gets the brightness of the requested display using the TCS34725 color sensors on the HMD Board
        /// 0 selects the left display from the HMD user's POV, and 1 selects the right display.
        /// Note that this is also affected by the HMD that is set. See SetHmd for more info.
        /// </summary>
        /// <param name="display">
        /// The display to read color from.
        ///     0 The left display from the perspective of wearing the HMD
        ///     1 The right display from the perspective of wearing the HMD
        /// </param>
        /// <returns>
        /// Returns the raw display brightness from the indicated display. The brightness is a magnitude between 0 and 65535. See the TCS34725 color sensor docs for more information. 
        /// </returns>
        public string GetDisplayBrightness(int display)
        {
            string brightness = string.Empty;
            try
            {
                this.SendCommand("GetDisplayBrightness", ResponseTimeColorSensor, display.ToString(), out brightness);
                return brightness;
            }
            catch
            {
                throw;
            }
        }

        /// <summary>
        /// Gets the RGB color values for the specified display. The colors are a magnitude between 0 and 65535. See the TCS34725 color sensor docs for more information. 
        /// </summary>
        /// <param name="display">
        /// The display to read color from.
        ///     0 The left display from the perspective of wearing the HMD
        ///     1 The right display from the perspective of wearing the HMD
        /// </param>
        /// <param name="red">
        /// Out param that returns the magnitude of the red component of the received light.
        /// </param>
        /// <param name="green">
        /// Out param that returns the magnitude of the green component of the received light.
        /// </param>
        /// <param name="blue">
        /// Out param that returns the magnitude of the blue component of the received light.
        /// </param>
        public void GetDisplayColor(int display, out string red, out string green, out string blue)
        {
            red = green = blue = string.Empty;
            string colorString = string.Empty;
            try
            {
                this.SendCommand("GetDisplayColor", ResponseTimeColorSensor, display.ToString(), out colorString);
                if (!string.IsNullOrEmpty(colorString))
                {
                    char[] delimiter = { ' ' };
                    string[] colors = colorString.Split(delimiter, StringSplitOptions.RemoveEmptyEntries);
                    if (colors.Length == 3)
                    {
                        red = colors[0];
                        green = colors[1];
                        blue = colors[2];
                    }
                }
            }
            catch
            {
                throw;
            }
        }

        /// <summary>
        /// Sets the number of volume samples to take before returning a result from any of the getVolume* methods. The default value is 2048 and the max is ULONG_MAX. THe sample rate of the Arduino is set to 38.4kHz.
        /// </summary>
        /// <param name="samples">
        /// The number of samples to collect before returning a volume level. One second is approximately 40,960 samples.
        /// </param>
        public void SetVolumeSampleCount(string samples)
        {
            try
            {
                this.SendCommand("SetVolumeSampleCount", ResponseTimeDefault, samples);

                // Adjust the response time to accommodate the new sample rate. Arduino sample frequency is 38.4kHz in the HMD Validation Kit firmware
                this.responseTimeGetVolume = ((int.Parse(samples) * 10) / 384) + 500;
            }
            catch
            {
                throw;
            }
        }

        /// <summary>
        /// Gets the RMS volume level on the 1/8" jack of the HMD exerciser over a number of samples. The result will be between 0 and 100*sqrt(2). The number of samples can be configured using setVolumeSampleCount.
        /// Volume levels are amplified on the PCB to give a full range response (0-5V) over headphone level signals. The center level is 2.5V.
        /// </summary>
        /// <returns>
        /// Returns the RMS volume over the given sample period on a scale of 0-100*sqrt(2), since the volume is not centered on zero.
        /// </returns>
        public string GetVolumeRms()
        {
            string volume = string.Empty;
            try
            {
                this.SendCommand("GetVolumeRms", this.responseTimeGetVolume, null, out volume);
                return volume;
            }
            catch
            {
                throw;
            }
        }

        /// <summary>
        /// Gets the peak volume level on the 1/8" jack of the HMD exerciser over a number of samples. The number of samples can be configured using setVolumeSampleCount.
        /// Volume levels are amplified on the PCB to give a full range response (0-5V) over headphone level signals. The center level is 2.5V.
        /// </summary>
        /// <returns>
        /// Returns the highest sample over the given sample period on a scale of 0-100.
        /// </returns>
        public string GetVolumePeak()
        {
            string volume = string.Empty;
            try
            {
                this.SendCommand("GetVolumePeak", this.responseTimeGetVolume, null, out volume);
                return volume;
            }
            catch
            {
                throw;
            }
        }

        /// <summary>
        /// Gets the average volume level on the 1/8" jack of the HMD exerciser over a number of samples. The number of samples can be configured using setVolumeSampleCount.
        /// Volume levels are amplified on the PCB to give a full range response (0-5V) over headphone level signals. The center level is 2.5V.
        /// </summary>
        /// <returns>
        /// Returns the average volume over the given sample period on a scale of 0-100.
        /// </returns>
        public string GetVolumeAvg()
        {
            string volume = string.Empty;
            try
            {
                this.SendCommand("GetVolumeAvg", this.responseTimeGetVolume, null, out volume);
                return volume;
            }
            catch
            {
                throw;
            }
        }

        /// <summary>
        /// Sets the HMD port that the other methods will act on. If the HMD of interest is plugged into P1, set the HMD to 1. if the HMD of interest is lugged into P2, set the HMD to 2.
        /// </summary>
        /// <param name="hmd">
        /// The HMD you would like brightness, presence, and color commands to apply to.
        ///     1 The HMD attached to port P1
        ///     2 The HMD attached to port P2
        /// </param>
        public void SetHmd(int hmd)
        {
            try
            {
                this.SendCommand("SetHmd", ResponseTimeDefault, hmd.ToString());
            }
            catch
            {
                throw;
            }
        }

        /// <summary>
        /// Gets the VBus voltage from the USB port
        /// </summary>
        /// <returns>
        /// Returns the USB voltage as displayed on the LCD
        /// </returns>
        public string Volts()
        {
            string volts = string.Empty;
            try
            {
                this.SendCommand("Volts", ResponseTimeDefault, null, out volts);
                return volts;
            }
            catch
            {
                throw;
            }
        }

        /// <summary>
        /// Gets the current drawn by the currently selected USB port.
        /// </summary>
        /// <returns>
        /// Returns the current as displayed on the LCD.
        /// </returns>
        public string Amps()
        {
            string amps = string.Empty;
            try
            {
                this.SendCommand("Amps", ResponseTimeDefault, null, out amps);
                return amps;
            }
            catch
            {
                throw;
            }
        }

        /// <summary>
        /// Sets the superspeed ports on or off. Pass true to enable superspeed lines and false to disable superspeed lines.
        /// </summary>
        /// <param name="status">
        /// The desired status of the SuperSpeed lines. 
        ///     True for connected
        ///     False for disconnected
        /// </param>
        public void SuperSpeed(bool status)
        {
            try
            {
                this.SendCommand("SuperSpeed", ResponseTimeDefault, status == true ? "1" : "0");
            }
            catch
            {
                throw;
            }
        }

        /// <summary>
        /// Sets a delay in seconds before the next issued command will execute.
        /// </summary>
        /// <param name="delay">
        /// The delay in seconds before the next command sent to the HMD kit will execute.
        /// </param>
        public void CommandDelay(int delay)
        {
            try
            {
                this.SendCommand("Delay", ResponseTimeDefault, delay.ToString());
            }
            catch
            {
                throw;
            }
        }

        /// <summary>
        /// Sets a timeout for the port to disconnect after the next connect command is sent
        /// </summary>
        /// <param name="timeout">
        /// The time in milliseconds the port will wait to disconnect after the next connect command is sent.
        /// </param>
        public void DisconnectTimeout(int timeout)
        {
            try
            {
                this.SendCommand("Timeout", ResponseTimeDefault, timeout.ToString());
            }
            catch
            {
                throw;
            }
        }

        /// <summary>
        /// Implementation of the Dispose interface to dispose of the managed resources used by the HmdKit class.
        /// </summary>
        /// <param name="disposing">
        /// If true, the managed resources are disposed.
        /// </param>
        protected virtual void Dispose(bool disposing)
        {
            if (disposing)
            {
                if (this.serialPort != null)
                {
                    this.serialPort.Dispose();
                }
            }
        }
    }
    #endregion
}
