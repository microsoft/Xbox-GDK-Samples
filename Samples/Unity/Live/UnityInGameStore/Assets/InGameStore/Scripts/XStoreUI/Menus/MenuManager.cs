using UnityEngine;

namespace GdkSample_InGameStore
{
    /// <summary>
    /// MenuManager class for managing menu transitions.
    /// </summary>
    public sealed class MenuManager : MonoBehaviour
    {
        [SerializeField] private GameObject MainMenuUI;
        [SerializeField] private GameObject ProductListMenuUI;
        [SerializeField] private GameObject ItemMenuUI;
        [SerializeField] private GameObject ConsoleUI;

        public static MenuManager Instance { get; private set; }

        private void Awake()
        {
            if (Instance != null)
            {
                Destroy(this);
                return;
            }

            Instance = this;
        }

        // Start is called before the first frame update
        void Start()
        {
            MainMenuUI.SetActive(true);
            ProductListMenuUI.SetActive(false);
            ItemMenuUI.SetActive(false);

            ConsoleUI.transform.SetParent(MainMenuUI.transform, false);
            ConsoleUI.SetActive(true);
        }

        public void ShowMainMenu()
        {
            MainMenuUI.SetActive(true);
            ProductListMenuUI.SetActive(false);
            ItemMenuUI.SetActive(false);

            ConsoleUI.transform.SetParent(MainMenuUI.transform, false);
            ConsoleUI.SetActive(true);

            MainMenu.Instance.ShowMenu();
        }

        public void ShowProductListMenu()
        {
            MainMenuUI.SetActive(false);            
            ProductListMenuUI.SetActive(true);
            ItemMenuUI.SetActive(false);

            // Move console to a non-canvas element so that it will not be
            // rendered but can still capture log messages.
            ConsoleUI.transform.SetParent(gameObject.transform, false);
            ConsoleUI.SetActive(true);

            ProductListMenu.Instance.ShowMenu();
        }

        public void ShowItemMenu(string storeId)
        {
            MainMenuUI.SetActive(false);
            ProductListMenuUI.SetActive(false);
            ItemMenuUI.SetActive(true);

            ConsoleUI.transform.SetParent(ItemMenuUI.transform, false);
            ConsoleUI.SetActive(true);

            ItemMenu.Instance.ShowMenu(storeId);
        }
    }
}
