namespace GdkSample_InGameStore
{
    // Common errors returned by XStore API.
    public enum E_XBOX_ERROR_CODES : int
    {
        // User already owns product.
        E_GAMESTORE_ALREADY_PURCHASED = -1994108156, //0x89245304

        // Could not resolve package identifier, is the package installed?
        E_GAMEPACKAGE_NO_PACKAGE_IDENTIFIER = -1994108408, //0x89245208

        // Item does not exist in catalog or is an invalid type.
        LM_E_CONTENT_NOT_IN_CATALOG = -2015294522, //0x87e10bc6

        // License for application is not valid for Store operations. If using a side-loaded/loose-deploy, check license set-up.
        IAP_E_BAD_LICENSE = -2143330040, //0x803f6108

        // Owner of package is not signed-in.
        LM_E_ENTITLED_USER_SIGNED_OUT = -2143318010, //0x803f9006

        // The specified account does not exist. Verify that the account is signed into the Xbox App.
        ERROR_NO_SUCH_USER = -2147023579, //0x80070525

        // Package identifier might be invalid, is the package installed?
        E_NOTSUPPORTED = -2147024846, //0x80070032

        // Process was canceled by user.
        E_ABORT = -2147467260 //0x80004004
    }

}
