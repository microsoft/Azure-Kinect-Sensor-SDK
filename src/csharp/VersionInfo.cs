//------------------------------------------------------------------------------
// <copyright file="VersionInfo.cs" company="Microsoft">
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
// </copyright>
//------------------------------------------------------------------------------
using System;
using System.Reflection;

// General Information about an assembly is controlled through the following
// set of attributes. Change these attribute values to modify the information
// associated with an assembly.
#if DEBUG
[assembly: AssemblyConfiguration("Debug")]
#else
[assembly: AssemblyConfiguration("Release")]
#endif

[assembly: AssemblyProduct("Azure Kinect")]
[assembly: AssemblyCompany("Microsoft")]
[assembly: AssemblyCopyright("Copyright (c) Microsoft Corporation. All rights reserved.")]
[assembly: AssemblyTrademark("")]

[assembly: CLSCompliant(true)]

// This is the shared assembly version file for all of our assemblies.
[assembly: AssemblyVersion("1.2.0")]
[assembly: AssemblyFileVersion("1.2.0-Beta.5")]
[assembly: AssemblyInformationalVersion("1.2.0-Informational")]