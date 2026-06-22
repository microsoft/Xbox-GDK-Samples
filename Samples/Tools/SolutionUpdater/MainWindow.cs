//-----------------------------------------------------------------------------
// MainWindow.cs - SolutionUpdater
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;
using System.Globalization;
using System.IO;
using System.Xml;

namespace SolutionUpdater
{
    public partial class MainWindow : Form
    {
        public MainWindow()
        {
            InitializeComponent();
        }

        // Fixed strings
        const string c_XboxOneXDKPlatform  = "Durango";
        const string c_XboxGDKPlatform     = "Gaming.Xbox.XboxOne.x64";

        const string c_ScarlettGDKPlatform = "Gaming.Xbox.Scarlett.x64";

        const string c_PCPlatform          = "x64";
        const string c_PC_GDKPlatform      = "Gaming.Desktop.x64";

        const string c_ProjectProcessed    = "Done";
              char[] trimProjectChars      = { '{', '}' };

        const string c_ProjectExtension    = ".vcxproj";
        const string c_ProjNewExtension    = ".backup.vcxproj";

        // Visual Studio Comment Hints
        const string c_VisualStudio_VersionHint       = "# Visual Studio ";
        const UInt32 c_VisualStudio2017_VersionNumber = 15;
        const UInt32 c_VisualStudio2017_ToolsetNumber = 141;

        public class FoundProjectInfo
        {
            public string m_ProjectPath;
            public bool   m_Processed;

            public FoundProjectInfo( string path )
            {
                m_ProjectPath = path;
                m_Processed = false;
            }
        }

        // Keep a list of found Projects in this Solution
        Dictionary<string, FoundProjectInfo> m_FoundProjects = new Dictionary<string, FoundProjectInfo>();
        

        bool ProcessSolutionFile(string inputSolutionFilename, bool backupEditedFiles)
        {
            // Start from a clean slate
            m_FoundProjects.Clear();

            // Store the path to the Solution File
            string pathToSolutionFile   = Path.GetDirectoryName( inputSolutionFilename );
            string tempSolutionFilename = Path.Combine( pathToSolutionFile, Path.GetFileNameWithoutExtension(inputSolutionFilename) ) + ".temp.sln";

            // Create the new Solution file
            using (StreamReader reader = new StreamReader( inputSolutionFilename, Encoding.UTF8 ))
            using (StreamWriter writer = new StreamWriter( tempSolutionFilename, false, Encoding.UTF8 ))
            {
            // Process the file line-by-line to swap out old config names
            string readLine;
            while ( ( readLine = reader.ReadLine() ) != null )
            {
                // Check for Project
                    if ( readLine.TrimStart().StartsWith( "Project(\"{", StringComparison.Ordinal ) )
                {
                    // Found a project file, so add it to the list
                    string[] splitLine = readLine.Split('"');
                    m_FoundProjects.Add( splitLine[7].Trim(trimProjectChars), new FoundProjectInfo(splitLine[5]) );
                }
                else
                {
                    // Check to see if we have already updated this Solution for the requested platform
                    if ( ( checkBoxXbox.Checked && readLine.Contains( "|" + c_XboxGDKPlatform ) ) ||
                         ( checkBoxPC.Checked && readLine.Contains( "|" + c_PC_GDKPlatform ) ) )
                    {
                        // Ooops ! We have already updated this Solution file, so message this and exit cleanly
                        MessageBox.Show( "This Solution file has already been updated to add the new platforms. Aborting this update.", "Already updated", MessageBoxButtons.OK, MessageBoxIcon.Exclamation, MessageBoxDefaultButton.Button1, 0 );

                        // Delete the temporary file
                        File.Delete( tempSolutionFilename );

                        return false;
                    }

                    // Check to see if this is the hint comment of which Visual Studio to load
                    if ( readLine.StartsWith( c_VisualStudio_VersionHint, StringComparison.Ordinal ) )
                    {
                        // Ensure that it is (at least) VS 2017
                        if ( Convert.ToInt32( readLine.Substring( readLine.Length - 2, 2 ), CultureInfo.InvariantCulture ) < c_VisualStudio2017_VersionNumber )
                        {
                            // Update it to ensure that the Solution will open in Visual Studio 2017
                            readLine = c_VisualStudio_VersionHint + c_VisualStudio2017_VersionNumber.ToString(CultureInfo.InvariantCulture);
                        }
                    }

                    // Update each requested platform - Order doesn't matter and the Solution file will be sorted when saving anyway
                    if ( checkBoxXbox.Checked && readLine.Contains( "|" + c_XboxOneXDKPlatform ) )
                    {
                        string newLine = readLine.Replace("|" + c_XboxOneXDKPlatform, "|" + c_XboxGDKPlatform);
                        writer.WriteLine( newLine );
                    }
                
                    if ( checkBoxPC.Checked && readLine.Contains( "|" + c_PCPlatform ) )
                    {
                        string newLine = readLine.Replace("|" + c_PCPlatform, "|" + c_PC_GDKPlatform);
                        writer.WriteLine( newLine );
                    }
                }

                // And output to the file
                writer.WriteLine( readLine );
            }

            } // end using reader, writer

            try
            {
                // Replace the original file with the updated one, keeping a backup copy if required
                File.Replace( tempSolutionFilename, inputSolutionFilename, backupEditedFiles?Path.Combine( pathToSolutionFile, Path.GetFileNameWithoutExtension(inputSolutionFilename) ) + ".backup.sln":null );
            }
            catch ( IOException e )
            {
                MessageBox.Show( "Error backing up \"" + inputSolutionFilename + "\" - Full error message below :\n\n" + e.InnerException, "File Error", MessageBoxButtons.OK, MessageBoxIcon.Error, MessageBoxDefaultButton.Button1, 0 );
                return false;
            }

            bool processedAnyProjectsThisLoop = false;

            // Then process the found project files in a loop until none are left unprocessed
            do
            {
                // Reset flag
                processedAnyProjectsThisLoop = false;
                
                Dictionary<string, FoundProjectInfo> newlyFoundProjects = new Dictionary<string, FoundProjectInfo>();

                // Process all of the project files in the global list
                foreach( FoundProjectInfo projectInfo in m_FoundProjects.Values )
                {
                    // Do we need to process this one ?
                    if ( projectInfo.m_Processed == false )
                    {
                        string projectFilenameAndPath = Path.Combine( pathToSolutionFile, projectInfo.m_ProjectPath );

                        // Check it exists before processing it
                        if ( File.Exists( projectFilenameAndPath ) )
                        {
                            if ( ProcessProjectFile( projectFilenameAndPath, ref newlyFoundProjects, backupEditedFiles ) != 0 )
                            {
                                // Quit here if we hit an error
                                return false;
                            }
                        }

                        // Mark it as processed
                        projectInfo.m_Processed = true;
                        processedAnyProjectsThisLoop = true;
                    }
                }

                // Update the found list by adding in any newly discovered project files
                foreach( KeyValuePair<string, FoundProjectInfo> kvp in newlyFoundProjects )
                {
                    // Does this exist already in the global list ?
                    if ( m_FoundProjects.ContainsKey( kvp.Key ) == false )
                    {
                        m_FoundProjects.Add( kvp.Key, kvp.Value );
                    }
                }
            }
            while ( processedAnyProjectsThisLoop );

            // All good
            return true;
        }

        class NewNodesToInsert
        {
            public XmlNode  newNode;
            public XmlNode  siblingToInsertAfter;

            // Basic Constructor
            public NewNodesToInsert( XmlNode node, XmlNode sibling )
            {
                newNode = node;
                siblingToInsertAfter = sibling;
            }
        }

        static void CreateNewPlatformNode ( XmlNode node, string platform, string newPlatform, ref List<XmlNode> nodeList )
        {
            // Duplicate the Node
            XmlNode newNode = node.CloneNode(true);

            // Update it
            newNode.Attributes["Include"].Value = node.Attributes["Include"].Value.Replace("|" + platform, "|" + newPlatform);

            // And the Child Platform node
            foreach (XmlNode platformNode in newNode.ChildNodes)
            {
                if (platformNode.Name == "Platform")
                {
                    platformNode.InnerText = platformNode.InnerText.Replace(platform, newPlatform);
                }
            }

            // Add it to the list for now
            nodeList.Add(newNode);
        }

        void ProcessProjectConfigurationsNode ( ref XmlNode parentNode )
        {
            // Create a temporary Node store to prevent a mutating list of Nodes in the Parent
            List<XmlNode> newNodeList = new List<XmlNode>();

            // Found it, process them all
            foreach (XmlNode configNode in parentNode.ChildNodes)
            {
                // Find all the requested nodes - Xbox
                if (checkBoxXbox.Checked && configNode.Attributes["Include"].Value.Contains("|" + c_XboxOneXDKPlatform))
                {
                    CreateNewPlatformNode( configNode, c_XboxOneXDKPlatform, c_XboxGDKPlatform, ref newNodeList );
                }
                // PC (x64)
                else if (checkBoxPC.Checked && configNode.Attributes["Include"].Value.Contains("|" + c_PCPlatform))
                {
                    CreateNewPlatformNode( configNode, c_PCPlatform, c_PC_GDKPlatform, ref newNodeList);
                }
            }

            // Now we can safely add the nodes from the temp list
            foreach ( XmlNode childNode in newNodeList )
            {
                parentNode.AppendChild( childNode );
            }
        }

        int ProcessProjectFile( string inputProjectFile, ref Dictionary<string, FoundProjectInfo> newlyFoundProjects, bool backupEditedFiles )
        {
            XmlDocument xmlDoc = new XmlDocument();
            xmlDoc.PreserveWhitespace = false;
            
            // Load the XML
            xmlDoc.Load(inputProjectFile);

            // Handle the namespace
            XmlNamespaceManager nsm = new XmlNamespaceManager(xmlDoc.NameTable);
            nsm.AddNamespace( "msb", xmlDoc.DocumentElement.NamespaceURI );

            // Grab the required node
            XmlNode projectNode = xmlDoc.DocumentElement;
            if ( projectNode == null || projectNode.Name != "Project" )
            {
                MessageBox.Show( "Couldn't find the top level <Project> Node. Are you sure this is a project file ?", "XML Error", MessageBoxButtons.OK, MessageBoxIcon.Stop, MessageBoxDefaultButton.Button1, 0 );
                return -1;
            }

            // Firstly update the ProjectConfigurations Node
            XmlNodeList configList = projectNode.SelectNodes("*[@Label='ProjectConfigurations']");

            // Check we have found only one
            if ( configList.Count != 1 )
            {
                MessageBox.Show( "Couldn't find the top level <ProjectConfigurations> Node. Are you sure this is a project file ?", "XML Error", MessageBoxButtons.OK, MessageBoxIcon.Stop, MessageBoxDefaultButton.Button1, 0 );
                return -2;
            }

            // Do the actual work
            XmlNode projConfigParent = configList[0];
            ProcessProjectConfigurationsNode( ref projConfigParent );
            
            // Temporary list for new nodes which will be added at the end of processing, before saving
            List<NewNodesToInsert> newChildNodes = new List<NewNodesToInsert>();

            // Now find the list of Condition Nodes to update
            XmlNodeList conditionNodeList = projectNode.SelectNodes("//*[@Condition]");

            // And Clone / Update them
            foreach ( XmlNode foundNode in conditionNodeList )
            {
                // Xbox Platform Checks
                if ( checkBoxXbox.Checked && ( foundNode.Attributes["Condition"].Value.Contains( "|" + c_XboxOneXDKPlatform ) ) )
                {
                    // Clone the Node
                    XmlNode newNode = foundNode.CloneNode( true );
                    newNode.Attributes["Condition"].Value = newNode.Attributes["Condition"].Value.Replace( c_XboxOneXDKPlatform, c_XboxGDKPlatform );

                    // Update any old references to Dependencies
                    XmlNodeList additionalDependencies = newNode.SelectNodes("//msb:AdditionalDependencies", nsm);
                    foreach (XmlNode depends in additionalDependencies)
                    {
                        // Remove old references, if they exist
                        depends.InnerText = depends.InnerText.Replace("kernel32.lib", "");
                        depends.InnerText = depends.InnerText.Replace("kernelx.lib", "");
                        depends.InnerText = depends.InnerText.Replace("d3d11_x.lib", "");
                        depends.InnerText = depends.InnerText.Replace("d3d12_x.lib", "");  // This Library is already included from the $(Console_Libs) dependency
                        depends.InnerText = depends.InnerText.Replace("combase.lib", "");
                        depends.InnerText = depends.InnerText.Replace("toolhelpx.lib", "");
                        depends.InnerText = depends.InnerText.Replace("etwplus.lib", "");

                        // Add new ones
                        depends.InnerText += "$(Console_Libs);%(XboxExtensionsDependencies);xmem.lib";

                        // Tidy up
                        depends.InnerText = depends.InnerText.Replace(";;", ";");
                        depends.InnerText = depends.InnerText.Trim(';');
                    }

                    // Update any old references to PreProcessor Defines
                    XmlNodeList preprocessorDefines = newNode.SelectNodes("//msb:PreprocessorDefinitions", nsm);
                    foreach (XmlNode defines in preprocessorDefines)
                    {
                        // Remove old references
                        defines.InnerText = defines.InnerText.Replace("WINAPI_FAMILY_TV_TITLE", "");
                        defines.InnerText = defines.InnerText.Replace("_XBOX_ONE", "");
                        defines.InnerText = defines.InnerText.Replace("_TITLE", "");
                        defines.InnerText = defines.InnerText.Replace("_DURANGO", "");

                        // Tidy up
                        defines.InnerText = defines.InnerText.Replace(";;", ";");
                        defines.InnerText = defines.InnerText.Trim(';');
                    }

                    // Update any references to older shader models
                    XmlNodeList shaderModels = newNode.SelectNodes("//msb:ShaderModel", nsm);
                    foreach (XmlNode smNode in shaderModels)
                    {
                        // Check that it is at least the minimum level supported
                        if ( Convert.ToDouble( smNode.InnerText, CultureInfo.InvariantCulture ) < 6.0 )
                        {
                            // Update to 6.0
                            smNode.InnerText = "6.0";
                        }
                    }

                    // Update any old references to previous (unsupported) Toolsets
                    XmlNodeList toolsetDefines = newNode.SelectNodes("//msb:PlatformToolset", nsm);
                    foreach (XmlNode toolset in toolsetDefines)
                    {
                        // Check it is at least Visual Studio 2017, as Visual Studio 2015 is unsupported for GXDK
                        if ( Convert.ToUInt32( toolset.InnerText.Substring(1), CultureInfo.InvariantCulture ) < c_VisualStudio2017_ToolsetNumber )
                        {
                            // Need to update to Visual Studio 2017
                            toolset.InnerText = "v" + c_VisualStudio2017_ToolsetNumber.ToString(CultureInfo.InvariantCulture);
                        }
                    }

                    // Remove any mentions of the /ZW flag
                    XmlNodeList zwFlags = newNode.SelectNodes("//msb:CompileAsWinRT", nsm);
                    foreach (XmlNode zwFlag in zwFlags)
                    {
                        // Remove the whole node
                        zwFlag.ParentNode.RemoveChild( zwFlag );
                    }

                    // Create the new Node and add it to the Dictionary for later insertion
                    NewNodesToInsert nodeToInsert = new NewNodesToInsert( newNode, foundNode );
                    newChildNodes.Add( nodeToInsert );
                }
                // x64 -> Gaming.Desktop.x64
                else if ( checkBoxPC.Checked && ( foundNode.Attributes["Condition"].Value.Contains( "|" + c_PCPlatform ) ) )
                {
                    // Clone the Node
                    XmlNode newNode = foundNode.CloneNode( true );
                    newNode.Attributes["Condition"].Value = newNode.Attributes["Condition"].Value.Replace( c_PCPlatform, c_PC_GDKPlatform );

                    // Update any old references to Dependencies
                    XmlNodeList additionalDependencies = newNode.SelectNodes("//msb:AdditionalDependencies", nsm);
                    foreach (XmlNode depends in additionalDependencies)
                    {
                        // Add new ones
                        depends.InnerText += "$(Console_Libs)";

                        // Tidy up
                        depends.InnerText = depends.InnerText.Replace(";;", ";");
                        depends.InnerText = depends.InnerText.Trim(';');
                    }

                    // Update any old references to PreProcessor Defines
                    XmlNodeList preprocessorDefines = newNode.SelectNodes("//msb:PreprocessorDefinitions", nsm);
                    foreach (XmlNode defines in preprocessorDefines)
                    {
                        // Remove old references
                        defines.InnerText = defines.InnerText.Replace("_WINDOWS", "");

                        // Add new ones
                        if (defines.InnerText.Contains("__WRL_NO_DEFAULT_LIB__") == false)
                        {
                            defines.InnerText += ";__WRL_NO_DEFAULT_LIB__";
                        }

                        // Tidy up
                        defines.InnerText = defines.InnerText.Replace(";;", ";");
                        defines.InnerText = defines.InnerText.Trim(';');
                    }

                    // Create the new Node and add it to the Dictionary for later insertion
                    NewNodesToInsert nodeToInsert = new NewNodesToInsert( newNode, foundNode );
                    newChildNodes.Add( nodeToInsert );
                }
            }

            // Check for any Referenced Projects
            XmlNodeList refProjects = projectNode.SelectNodes("//msb:ProjectReference", nsm);

            // And Clone / Update them
            foreach( XmlNode foundNode in refProjects )
            {
                // Check to see if this is the node type we require
                if ( foundNode.Attributes["Include"] != null )
                {
                    // Find the Project GUID
                    string guid = foundNode.SelectSingleNode("msb:Project", nsm).InnerText;
                    string projFile = foundNode.Attributes["Include"].InnerText;

                    // Add them to the Dictionary
                    if ( newlyFoundProjects.ContainsKey( guid ) == false )
                    {
                        newlyFoundProjects.Add( guid, new FoundProjectInfo(projFile) );
                    }
                }
            }

            // Add the newly created nodes into the Document in the correct locations
            foreach( NewNodesToInsert node in newChildNodes )
            {
                // Add it after the copied node
                node.siblingToInsertAfter.ParentNode.InsertAfter(node.newNode, node.siblingToInsertAfter);
            }

            // All done, so save it, keeping a backup if required
            if ( backupEditedFiles )
            {
                try
                {
                    // Keep a copy of the original Project file
                    File.Copy( inputProjectFile, inputProjectFile.Replace( c_ProjectExtension, c_ProjNewExtension ), true );
                }
                catch ( IOException e )
                {
                    // Ooops !
                    MessageBox.Show( "Error backing up \"" + inputProjectFile + "\" - Full error message below :\n\n" + e.InnerException, "File Error", MessageBoxButtons.OK, MessageBoxIcon.Error, MessageBoxDefaultButton.Button1, 0 );
                    return -2;
                }
            }

            // And save the edited document
            xmlDoc.Save( inputProjectFile );
            
            // Check if we need 
            return 0;
        }

#region EventProcessing

        private void OpenSolutionButton_Click(object sender, EventArgs e)
        {
            currentSolutionFilename.Text = null;

            // Show dialog and find solution
            if ( openSLNFileDialog.ShowDialog() == DialogResult.OK )
            {
                // Update the label
                currentSolutionFilename.Text = openSLNFileDialog.FileName;
            }
        }

        private void StartUpdate_Click(object sender, EventArgs e)
        {
            // Check that at least one box is checked
            if ( ( checkBoxXbox.Checked || checkBoxPC.Checked ) == false )
            {
                // Message User
                MessageBox.Show( "You must select at least one Platform to Update", "Selection Error", MessageBoxButtons.OK, MessageBoxIcon.Information, MessageBoxDefaultButton.Button1, 0 );
                return;
            }

            // Ensure that the user knows that it will overwrite the original files
            if ( backupEditedFilesCheckbox.Checked == false )
            {
                // Check that they wish to continue
                if ( MessageBox.Show( "You have selected *NOT* to keep backups of the edited files.\n\nPlease ensure that you have some means of recovering the edited files in case of any issues (SourceControl or manual backups etc.)\n\nPress \"OK\" to proceed.",
                                      "Backup warning", MessageBoxButtons.OKCancel, MessageBoxIcon.Warning, MessageBoxDefaultButton.Button1, 0 ) != DialogResult.OK )
                {
                    // User has canceled
                    return;
                }
            }

            // Final check
            if ( File.Exists( currentSolutionFilename.Text ) )
            {
                // Freeze the window while we are updating to prevent user changes
                startUpdate.Enabled         = false;
                OpenSolutionButton.Enabled  = false;
                checkBoxPC.Enabled          = false;
                checkBoxXbox.Enabled        = false;
                backupEditedFilesCheckbox.Enabled   = false;

                // Call the function, checking if they want to overwrite the original file or not
                if ( ProcessSolutionFile( currentSolutionFilename.Text, backupEditedFilesCheckbox.Checked ) )
                {
                    // Inform the user that we are done
                    MessageBox.Show( "All Updates Completed", "Done !", MessageBoxButtons.OK, MessageBoxIcon.Information, MessageBoxDefaultButton.Button1, 0 );
                }

                // All done - Unfreeze
                startUpdate.Enabled         = true;
                OpenSolutionButton.Enabled  = true;
                checkBoxPC.Enabled          = true;
                checkBoxXbox.Enabled        = true;
                backupEditedFilesCheckbox.Enabled   = true;
            }

            return;
        }

#endregion
    }
}
