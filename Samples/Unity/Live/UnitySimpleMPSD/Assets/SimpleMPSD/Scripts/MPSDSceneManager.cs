using System.Collections.Generic;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;

namespace GdkSample_SimpleMPSD
{
    public sealed class MPSDSceneManager : MonoBehaviour
    {
        public SessionManager SessionManager;
        public EventSystem EventSystem;

        [SerializeField] private Button HostSessionButton;
        [SerializeField] private Button JoinSessionButton;
        [SerializeField] private Button InviteFriendButton;
        [SerializeField] private Button LeaveSessionButton;
        [SerializeField] private Button StartMatchmakingButton;
        [SerializeField] private Button CancelMatchmakingButton;

        private readonly List<Button> _buttons = new();
        private string _joinableSessionHandleId = null;


        // Start is called before the first frame update
        private void Start()
        {
            XboxManager.Instance.UserSignedIn += HandleUserSignedIn;
            XboxManager.Instance.UserSignedOut += HandleUserSignedOut;

            InitializeButtons();

            SetButtonInteractableState(false);

            // Initialize GDK runtime and Xbox Live
            if (GDKGameRuntime.TryInitialize())
            {
                XboxManager.Instance.InitializeAndAddUser();
            }

            if (SessionManager == null)
            {
                SessionManager = gameObject.AddComponent<SessionManager>();
            }
        }

        public void HandleHostSession()
        {
            Logger.Instance.Log("Hosting session...", LogColor.Event);

            HostSessionButton.interactable = false;
            InviteFriendButton.interactable = false;
            LeaveSessionButton.interactable = false;

            SessionManager.CreateSession(SessionConfiguration.GAME_SESSION_TEMPLATE_NAME);
        }

        public void HandleJoinSession()
        {
            Logger.Instance.Log("Joining session...", LogColor.Event);

            JoinSessionButton.interactable = false;
            LeaveSessionButton.interactable = false;

            SessionManager.JoinSession(_joinableSessionHandleId);
        }

        public void HandleInviteFriend()
        {
            Logger.Instance.Log("Inviting friend...", LogColor.Event);

            InviteFriendButton.interactable = false;
            LeaveSessionButton.interactable = false;

            SessionManager.ShowPlatformInviteUI(XboxManager.Instance.UserHandle);

            InviteFriendButton.interactable = true;
            LeaveSessionButton.interactable = true;

            EventSystem.SetSelectedGameObject(LeaveSessionButton.gameObject);
            RefreshButtonNavigation();
        }

        public void HandleLeaveSession()
        {
            Logger.Instance.Log("Leaving session...", LogColor.Event);

            HostSessionButton.interactable = false;
            InviteFriendButton.interactable = false;
            LeaveSessionButton.interactable = false;

            SessionManager.LeaveSession();
        }

        public void HandleStartMatchmaking()
        {
            Logger.Instance.Log("Starting matchmaking...", LogColor.Event);

            HostSessionButton.interactable = false;
            StartMatchmakingButton.interactable = false;
            CancelMatchmakingButton.interactable = true;

            EventSystem.SetSelectedGameObject(CancelMatchmakingButton.gameObject);
            RefreshButtonNavigation();

            SessionManager.MatchmakeSession(
                SessionConfiguration.LOBBY_SESSION_TEMPLATE_NAME,
                SessionConfiguration.MATCHMAKE_HOPPER_NAME);

            Invoke(nameof(HandleCancelMatchmaking), SessionConfiguration.MATCHMAKE_TIMEOUT_S);
        }

        public void HandleCancelMatchmaking()
        {
            if(CancelMatchmakingButton.interactable)
            {
                Logger.Instance.Log("Cancelling matchmaking...", LogColor.Event);
                CancelMatchmakingButton.interactable = false;
                SessionManager.CancelMatchmake(SessionConfiguration.MATCHMAKE_HOPPER_NAME);
            }
        }

        /// <summary>
        /// Called after an Xbox Live user has signed into the game
        /// </summary>
        private void HandleUserSignedIn()
        {
            HostSessionButton.interactable = true;
            StartMatchmakingButton.interactable = true;

            SessionManager.OnCreateSessionCompleted += HandleSessionCreated;
            SessionManager.OnJoinSessionCompleted += HandleSessionJoined;
            SessionManager.OnFindSessionCompleted += HandleSessionFound;
            SessionManager.OnLeaveSessionCompleted += HandleSessionLeft;
            SessionManager.OnSessionInviteReceived += HandleInviteReceived;

            SessionManager.Init();
        }

        /// <summary>
        /// Called when the active Xbox Live user signs out of the system
        /// </summary>
        private void HandleUserSignedOut()
        {
            SetButtonInteractableState(false);
        }

        private void HandleSessionCreated(bool success)
        {
            JoinSessionButton.interactable &= !success;
            HostSessionButton.interactable = !success;
            StartMatchmakingButton.interactable = !success;
            InviteFriendButton.interactable = success;
            LeaveSessionButton.interactable = success;

            if (success)
            {
                _joinableSessionHandleId = null;
                EventSystem.SetSelectedGameObject(LeaveSessionButton.gameObject);
            }
            else
            {
                EventSystem.SetSelectedGameObject(HostSessionButton.gameObject);
            }

            RefreshButtonNavigation();
        }

        private void HandleSessionJoined(bool success)
        {
            _joinableSessionHandleId = null;

            HostSessionButton.interactable = !success;
            StartMatchmakingButton.interactable = !success;
            InviteFriendButton.interactable = success;
            LeaveSessionButton.interactable = success;

            if (success)
            {
                EventSystem.SetSelectedGameObject(LeaveSessionButton.gameObject);
            }
            else
            {
                EventSystem.SetSelectedGameObject(HostSessionButton.gameObject);
            }

            RefreshButtonNavigation();
        }

        private void HandleSessionFound(bool success)
        {
            JoinSessionButton.interactable &= !success;
            HostSessionButton.interactable = !success;
            StartMatchmakingButton.interactable = !success;
            InviteFriendButton.interactable = success;
            LeaveSessionButton.interactable = success;
            CancelMatchmakingButton.interactable = false;

            if (success)
            {
                _joinableSessionHandleId = null;
                EventSystem.SetSelectedGameObject(LeaveSessionButton.gameObject);
            }
            else
            {
                EventSystem.SetSelectedGameObject(StartMatchmakingButton.gameObject);
            }

            RefreshButtonNavigation();
        }

        private void HandleSessionLeft(bool success)
        {
            HostSessionButton.interactable = success;
            StartMatchmakingButton.interactable = success;
            InviteFriendButton.interactable &= !success;
            LeaveSessionButton.interactable = !success;

            if (success)
            {
                EventSystem.SetSelectedGameObject(HostSessionButton.gameObject);
            }
            else
            {
                EventSystem.SetSelectedGameObject(LeaveSessionButton.gameObject);
            }

            RefreshButtonNavigation();
        }

        private void HandleInviteReceived(string sessionHandleId)
        {
            if (!SessionManager.IsInSession())
            {
                _joinableSessionHandleId = sessionHandleId;
                JoinSessionButton.interactable = true;
                EventSystem.SetSelectedGameObject(JoinSessionButton.gameObject);
                RefreshButtonNavigation();
            }
        }

        private void InitializeButtons()
        {
            _buttons.Add(HostSessionButton);
            _buttons.Add(JoinSessionButton);
            _buttons.Add(InviteFriendButton);
            _buttons.Add(LeaveSessionButton);
            _buttons.Add(StartMatchmakingButton);
            _buttons.Add(CancelMatchmakingButton);
        }

        private void SetButtonInteractableState(bool isInteractable)
        {
            foreach (Button button in _buttons)
            {
                button.interactable = isInteractable;
            }
        }

        private void RefreshButtonNavigation()
        {
            var enabledOnes = new List<Button>();
            var disabledOnes = new List<Button>();

            foreach (var button in _buttons)
            {
                if (button.interactable)
                {
                    enabledOnes.Add(button);
                }
                else
                {
                    disabledOnes.Add(button);
                }
            }

            foreach (var disabledButton in disabledOnes)
            {
                var nav = disabledButton.navigation;
                nav.selectOnUp = null;
                nav.selectOnDown = null;
                disabledButton.navigation = nav;
            }

            for (var i = 0; i < enabledOnes.Count; i++)
            {
                var enabledButton = enabledOnes[i];
                var nav = enabledButton.navigation;

                if (i == 0)
                {
                    nav.selectOnUp = null;
                }
                else
                {
                    nav.selectOnUp = enabledOnes[i - 1];
                }

                if (i == enabledOnes.Count - 1)
                {
                    nav.selectOnDown = enabledOnes[0];
                }
                else
                {
                    nav.selectOnDown = enabledOnes[i + 1];
                }

                enabledButton.navigation = nav;
            }
        }

    }
} // namespace 