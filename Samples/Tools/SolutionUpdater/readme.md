# SolutionUpdater Sample

*This sample is compatible with the Microsoft Game Development Kit (March 2022)*

# Description

This sample tool is provided as means to move a large number of Visual
Studio Projects to the new Game Core programming model. The tool takes a
Visual Studio Solution file (.sln) as input and uses this to \"walk\"
the associated Projects adding in the newly required platforms as it
progresses. The tool can be used to update PC only, Xbox only or both
platforms simultaneously.

The default behavior of the tool is to **overwrite** any updated files
(Solution and Project files) so please ensure that you can recover the
original files from Source Control, or a manual backup, in the case of
any errors. The tool does provide a "Backup Edited Files\" checkbox
which will create backup files as it goes, but this should not be relied
on as a complete backup strategy.

Running this tool provides the first step in moving your title over to
Game Core and is **not** intended on providing a \"one-click\" method
for conversion. Please refer to the \"Porting Guide\" in the
GDK documentation for further details on the additional work required to
comply with the new programming model.

# Usage

When you run the tool you are presented with a simple UI, shown below :

![](./media/image1.png)

Click on the "Open" button to select the Solution file to convert. Then
select the options that apply to the platforms that you develop for,
deselecting the "Backup" option if you are confident about the update.

The tool will then process all of the required files and show a message
on success or failure, with the thrown exception information.

Then you can load the newly updated Solution file in Visual Studio to
review the changes that the tool has made.

# Update history

Initial release July 2019.
