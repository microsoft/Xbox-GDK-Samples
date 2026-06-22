Due to the directory structure required to house the multiple Visual Studio project files for this sample, 
additional metadata is required for each asset included in the sample's build. Each asset must be given a 
<Link> metadata tag in its corresponding entry in the .vcxproj file. The result looks like this:

    <Image Include="..\Assets\Store\SplashScreen.png">
      <Link>Assets\Store\%(Filename)%(Extension)</Link>
    </Image>

The link serves to preserve the Assets directory structure in the build output directory. (Without this link, 
Visual Studio would encounter the ".." at the beginning of the include path and ignore the remaining directory 
structure of the asset file, causing the asset file to be placed in the root of the build output.)

Since no standard UI is currently available in Visual Studio for editing this metadata, we have provided our 
own customization. This build customization is imported by AssetLink.targets and defined in AssetLink.xml. 
By including this customization in the project files, we have enabled you to enter the Link metadata in the 
file Properties UI in Visual Studio. Simply right-click on an asset file, select Properties, and edit the 
"Asset Link" field under the "Metadata Extensions" property. Typically, you would want to preserve the original 
directory path where your asset loading source code will expect to find this asset, which can be generalized by 
using the well-known %(Filename)%(Extension) properties in the path.
