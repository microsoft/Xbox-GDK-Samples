#include "imgui_acc_gdk.h"

#pragma region Narrator
Narrator::Narrator(XTaskQueueHandle taskqueue) :
    managementThreadActive(false),
    isManagingCOMLibrary(false),
    isNarrationEnabled(true)
{
    // Initialize COM library if not already initialized
    HRESULT hr = CoInitializeEx(NULL, COINITBASE_MULTITHREADED);
    bool comInitialized = SUCCEEDED(hr) && hr != S_FALSE;
    if (comInitialized)
    {
        isManagingCOMLibrary = true;
    }
    else if (hr != S_FALSE)
    {
        throw std::runtime_error("COM Library failed to initialize");
        return;
    }

    // Initialize XAudio2
    hr = XAudio2Create(&pXAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
    if (FAILED(hr))
    {
        throw std::runtime_error("XAudio2 failed to initialize");
        return;
    }

    // Create mastering voice. It encapsulates an audio device
    hr = pXAudio2->CreateMasteringVoice(&pMasterVoice);
    if (FAILED(hr))
    {
        throw std::runtime_error("Mastering voice failed to initialize");
        return;
    }

    waveFormat.wFormatTag = WAVE_FORMAT_PCM;
    waveFormat.nChannels = 1;
    waveFormat.nSamplesPerSec = 22050;
    waveFormat.nAvgBytesPerSec = 22050 * sizeof(short);
    waveFormat.nBlockAlign = sizeof(short);
    waveFormat.wBitsPerSample = 16;
    waveFormat.cbSize = 0;

    hr = pXAudio2->CreateSourceVoice(&pSourceVoice, &waveFormat);
    if (FAILED(hr))
    {
        throw std::runtime_error("Source voice failed to initialize");
        return;
    }

    keepNarrationThreadAlive = true;

    XAsyncBlock* async = new XAsyncBlock{};
    async->queue = taskqueue;
    async->context = this;

    XAsyncRun(async,
        [](XAsyncBlock* asyncBlock)->HRESULT
        {
            Narrator* pThis = reinterpret_cast<Narrator*>(asyncBlock->context);

            // Speech synthesizer stream related variables
            size_t bufferSize;
            std::vector<char> streamData;
            XAUDIO2_BUFFER audioBuffer = {};
            XSpeechSynthesizerHandle ssHandle = nullptr;
            XSpeechSynthesizerStreamHandle ssStreamHandle = nullptr;

            // Create the speech synthesizer used to convert text to audio data
            HRESULT hr = XSpeechSynthesizerCreate(&ssHandle);
            if (FAILED(hr))
            {
                throw std::runtime_error("Speech synthesizer failed to initialize");
                return E_FAIL;
            }

            // Every 100ms, check if the value of the current focused widget has changed.
            // If so, stop any previous narration and start narrating the new value.
            while (pThis->keepNarrationThreadAlive)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(Narrator::DELAYMS));

                // Acquire the narrationMutex to safely check if the current focused widget differs from the last narrated text
                pThis->narrationMutex.lock();
                if (pThis->isNarrationEnabled) {
                    if (pThis->currentWidgetValue != pThis->lastCompleteNarration)
                    {
                        // Update the last narrated text and free the mutex
                        pThis->lastCompleteNarration = pThis->currentWidgetValue;
                        pThis->narrationMutex.unlock();

                        // Stop any active narration
                        XAUDIO2_VOICE_STATE state;
                        pThis->pSourceVoice->GetState(&state);
                        if (state.BuffersQueued > 0)
                        {
                            pThis->pSourceVoice->Stop();
                            pThis->pSourceVoice->FlushSourceBuffers();
                        }

                        if (FAILED(XSpeechSynthesizerCreateStreamFromText(ssHandle, pThis->currentWidgetValue.c_str(), &ssStreamHandle)))
                        {
                            error_status_t status = GetLastError();
                            std::string errorMessage = "XSpeechSynthesizerCreateStreamFromText failed with error code: " + std::to_string(status);
                            throw std::runtime_error(errorMessage);
                            return E_FAIL;
                        }
                        if (FAILED(XSpeechSynthesizerGetStreamDataSize(ssStreamHandle, &bufferSize)))
                        {
                            error_status_t status = GetLastError();
                            std::string errorMessage = "XSpeechSynthesizerGetStreamDataSize failed with error code: " + std::to_string(status);
                            throw std::runtime_error(errorMessage);
                            return E_FAIL;
                        }

                        streamData.resize(bufferSize);
                        if (FAILED(XSpeechSynthesizerGetStreamData(ssStreamHandle, bufferSize, streamData.data(), &bufferSize)))
                        {
                            error_status_t status = GetLastError();
                            std::string errorMessage = "XSpeechSynthesizerGetStreamData failed with error code: " + std::to_string(status);
                            OutputDebugStringA(errorMessage.c_str());
                            return E_FAIL;
                        }


                        // Convert the synth stream data to an XAudio2 buffer.
                        audioBuffer.AudioBytes = static_cast<UINT32>(bufferSize);
                        audioBuffer.pAudioData = reinterpret_cast<const BYTE*>(streamData.data());

                        // Submit Xaudio2 buffer to source voice
                        if (FAILED(pThis->pSourceVoice->SubmitSourceBuffer(&audioBuffer)))
                        {
                            error_status_t status = GetLastError();
                            std::string errorMessage = "SubmitSourceBuffer failed with error code: " + std::to_string(status);
                            OutputDebugStringA(errorMessage.c_str());
                            return E_FAIL;
                        }

                        // Start the source voice
                        if (FAILED(pThis->pSourceVoice->Start(0)))
                        {
                            error_status_t status = GetLastError();
                            std::string errorMessage = "IXAudio2SourceVoice start failed with error code: " + std::to_string(status);
                            OutputDebugStringA(errorMessage.c_str());
                            return E_FAIL;
                        }
                    }
                    else
                    {
                        pThis->narrationMutex.unlock();

                        // Check if the source voice is done playing, close the stream handle
                        XAUDIO2_VOICE_STATE state;
                        pThis->pSourceVoice->GetState(&state);

                        if (ssStreamHandle != nullptr && state.BuffersQueued == 0)
                        {
                            XSpeechSynthesizerCloseStreamHandle(ssStreamHandle);
                            ssStreamHandle = nullptr;
                        }
                    }
                }
                else
                {
                    // Narration is disabled, stop any active narration and reset state
                    XAUDIO2_VOICE_STATE state;
                    pThis->pSourceVoice->GetState(&state);
                    if (state.BuffersQueued > 0)
                    {
                        pThis->pSourceVoice->Stop();
                        pThis->pSourceVoice->FlushSourceBuffers();
                    }
                    pThis->lastCompleteNarration = "";
                    pThis->narrationMutex.unlock();
                }
            }

            if(ssStreamHandle != nullptr){
                XSpeechSynthesizerCloseStreamHandle(ssStreamHandle);
            }
            if(ssHandle != nullptr){
                XSpeechSynthesizerCloseHandle(ssHandle);
            }

            return S_OK;
        });
}

Narrator::~Narrator()
{
    keepNarrationThreadAlive = false;

    // Cleanup XAudio2 Resources
    XAUDIO2_VOICE_STATE state;
    pSourceVoice->GetState(&state);
    if (state.BuffersQueued > 0)
    {
        pSourceVoice->Stop();
        pSourceVoice->FlushSourceBuffers();
    }
    pSourceVoice->DestroyVoice();
    pMasterVoice->DestroyVoice();
    pXAudio2->Release();

    if (isManagingCOMLibrary)
    {
        CoUninitialize();
    }
}

void Narrator::AddPendingNarration(const char* newValue)
{
    // Update the current widget value.
    // The narrator management thread will pick up the change and narrate it.
    {
        std::lock_guard<std::mutex> lock(narrationMutex);
        currentWidgetValue = newValue;
    }
}

void Narrator::EnableNarration()
{
    {
        std::lock_guard<std::mutex> lock(narrationMutex);
        isNarrationEnabled = true;
    }
}

void Narrator::DisableNarration()
{
    {
        std::lock_guard<std::mutex> lock(narrationMutex);
        isNarrationEnabled = false;
    }
}

#pragma endregion

#pragma region Base class
imgui_acc_gdk::imgui_acc_gdk():
    m_scaleFactor(0),
    m_narrator(nullptr),
    m_keyboardManager(nullptr),
    m_taskQueue(nullptr)
{
}

imgui_acc_gdk::~imgui_acc_gdk()
{
    if (m_taskQueue)
    {
        XTaskQueueTerminate(m_taskQueue, false, nullptr, nullptr);
        XTaskQueueDispatch(m_taskQueue, XTaskQueuePort::Work, INFINITE);
        XTaskQueueDispatch(m_taskQueue, XTaskQueuePort::Completion, INFINITE);
        XTaskQueueCloseHandle(m_taskQueue);
    }

    if(m_narrator)
        delete m_narrator;

    if(m_keyboardManager)
        delete m_keyboardManager;
}

imgui_acc_gdk* imgui_acc_gdk::GetInstance()
{
    if(instance == nullptr)
    {
        instance = new imgui_acc_gdk();
    }

    return instance;
}

HRESULT imgui_acc_gdk::Initialize()
{
    DetectHighContrastTheme();

    HRESULT hr = XTaskQueueCreate(
        XTaskQueueDispatchMode::ThreadPool,
        XTaskQueueDispatchMode::ThreadPool,
        &m_taskQueue);

    if (FAILED(hr))
    {
        return hr;
    }

    m_scaleFactor = 1.0f;
    m_narrator = new Narrator(m_taskQueue);
    m_keyboardManager = new VirtualKeyboardManager();
    return S_OK;
}

void imgui_acc_gdk::CleanUp()
{
    delete instance;
    instance = nullptr;
}

void imgui_acc_gdk::NewFrame()
{
    DetectHighContrastTheme();
}

#pragma endregion

#pragma region Imgui Widget Wrappers
bool imgui_acc_gdk::Begin(const char* name, bool* p_open, ImGuiWindowFlags flags, ImVec2 size)
{
    ImGui::SetNextWindowSize(size);

    if (!ImGui::Begin(name, p_open, flags | ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize))
        return false;

    // Enable horizontal scrolling
    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootWindow) && !ImGui::IsAnyItemActive())
    {
        if (ImGui::IsKeyDown(ImGuiKey_GamepadLStickLeft))
        {
            ImGui::SetScrollX(ImGui::GetScrollX() - 10.0f);
        }
        if (ImGui::IsKeyDown(ImGuiKey_GamepadLStickRight))
        {
            ImGui::SetScrollX(ImGui::GetScrollX() + 10.0f);
        }
    }

    return true;
}

void imgui_acc_gdk::End()
{
    ImGui::End();
}

void imgui_acc_gdk::Text(const char* text, ...)
{
    // ImGui::Text is not interactable at all, and for the purposes of narration, not useable.
    // Selectable is used in its place instead.
    ImGui::Selectable(text);
    if (ImGui::IsItemFocused())
    {
        m_narrator->AddPendingNarration(text);
    }
}

bool imgui_acc_gdk::Button(const char* label, const ImVec2&)
{
    ImGuiStyle& style = ImGui::GetStyle();

    // Calculate the size ImGui would normally use
    ImVec2 size =
    {
        ImGui::CalcTextSize(label, NULL, true).x + style.FramePadding.x * 2.0f,
        ImGui::CalcTextSize(label, NULL, true).y + style.FramePadding.y * 2.0f
    };

    // Enforce minimum size
    ImVec2 adjusted_size;
    adjusted_size.x = max(size.x, MINWIDGETSIZE.x);
    adjusted_size.y = max(size.y, MINWIDGETSIZE.y);

    bool pressed = ImGui::Button(label, adjusted_size);

    if (ImGui::IsItemFocused())
{
        m_narrator->AddPendingNarration(label);
    }

    char buffer[256] = {};
    if(ImGui::IsItemDeactivated())
    {
        sprintf_s(buffer, 256, u8"%s %s\n", label, "pressed.");
        m_narrator->AddPendingNarration(buffer);
    }
    return pressed;
}

bool imgui_acc_gdk::InputText(const char* label, char* buf, size_t buf_size, ImGuiInputTextFlags flags, GameInputKind activeGameInputKind)
{
    bool valueChanged = false;

    // If physical keyboard is not connected, use virtual keyboard
    if ((activeGameInputKind & GameInputKind::GameInputKindKeyboard) == 0)
    {
        // Callback is called every frame when the input text field is active
        ImGuiInputTextCallback keyboardCallback = [](ImGuiInputTextCallbackData* data) -> int
            {
                imgui_acc_gdk* pThis = static_cast<imgui_acc_gdk*>(data->UserData);
                if (data->EventFlag == ImGuiInputTextFlags_CallbackAlways)
                {
                    if (pThis->m_keyboardManager->IsKeyboardActive())
                    {
                        uint32_t cursorPosition;
                        XGameUiTextEntryChangeTypeFlags changeType;

                        char updatedBuffer[MAXBUFFERSIZE] = {};
                        XGameUiTextEntryGetState(
                            pThis->m_keyboardManager->keyboardHandle,
                            &changeType,
                            &cursorPosition,
                            nullptr,
                            nullptr,
                            MAXBUFFERSIZE,
                            updatedBuffer);

                        if (static_cast<uint32_t>(XGameUiTextEntryChangeTypeFlags::TextChanged) & static_cast<uint32_t>(changeType))
                        {
                            // Narrate the latest text
                            char speechBuffer[MAXBUFFERSIZE] = {};
                            sprintf_s(speechBuffer, MAXBUFFERSIZE, u8"%s", updatedBuffer);
                            pThis->m_narrator->AddPendingNarration(speechBuffer);

                            data->DeleteChars(0, data->BufTextLen);
                            data->InsertChars(0, updatedBuffer);
                        }
                    }
                    else
                    {
                        pThis->m_keyboardManager->ActivateKeyboard(data->Buf);
                    }
                }
                return 0;
            };

        ImGuiStyle& style = ImGui::GetStyle();
        // Calculate the default size
        ImVec2 text_size = ImGui::CalcTextSize(buf, NULL, true);
        ImVec2 total_size;
        total_size.x = text_size.x + style.FramePadding.x * 2.0f;
        total_size.y = text_size.y + style.FramePadding.y * 2.0f;

        // Enforce minimum size
        ImVec2 size_diff;
        size_diff.x = max(MINWIDGETSIZE.x - total_size.x, 0.0f);
        size_diff.y = max(MINWIDGETSIZE.y - total_size.y, 0.0f);

        // Adjust frame padding if necessary
        if (size_diff.x > 0.0f || size_diff.y > 0.0f)
        {
            ImVec2 padding = { 0,0 };
            padding.x = style.FramePadding.x + size_diff.x / 2.0f;
            padding.y = style.FramePadding.y + size_diff.y / 2.0f;
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, padding);
        }

        valueChanged = ImGui::InputText(label, buf, buf_size, flags | ImGuiInputTextFlags_CallbackAlways, keyboardCallback, this);

        if (size_diff.x > 0.0f || size_diff.y > 0.0f)
        {
            ImGui::PopStyleVar();
        }

        char buffer[MAXBUFFERSIZE] = {};
        if (ImGui::IsItemActive())
        {
            sprintf_s(buffer, MAXBUFFERSIZE, u8"Editing %s %s\n", label, buf);
            m_narrator->AddPendingNarration(buffer);
        }
        else if (ImGui::IsItemFocused())
        {
            sprintf_s(buffer, MAXBUFFERSIZE, u8"%s %s\n", label, buf);
            m_narrator->AddPendingNarration(buffer);
        }

        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            // Check if deactivation was due to ImGuiKey_GamepadBack. If so, force a strcpy of the latest
            // virtual keyboard text into the ImGui buffer.
            if (ImGui::IsKeyPressed(ImGuiKey_GamepadFaceRight))
            {
                uint32_t cursorPosition;
                XGameUiTextEntryChangeTypeFlags changeType;
                char updatedBuffer[MAXBUFFERSIZE] = {};
                XGameUiTextEntryGetState(
                    m_keyboardManager->keyboardHandle,
                    &changeType,
                    &cursorPosition,
                    nullptr,
                    nullptr,
                    MAXBUFFERSIZE,
                    updatedBuffer);
                strcpy_s(buf, MAXBUFFERSIZE, updatedBuffer);
            }
            m_keyboardManager->DeactivateKeyboard();
        }
        else if (ImGui::IsItemDeactivated())
        {
            m_keyboardManager->DeactivateKeyboard();
        }
    }
    else
    {
        // Physical keyboard is connected, so deactivate the virtual keyboard
        if (m_keyboardManager->IsKeyboardActive())
        {
            m_keyboardManager->DeactivateKeyboard();
        }

        valueChanged = ImGui::InputText(label, buf, buf_size, flags);
        if (ImGui::IsItemFocused())
        {
            char buffer[256] = {};
            sprintf_s(buffer, 256, u8"%s %s\n", label, buf);
            m_narrator->AddPendingNarration(buffer);
        }
    }
    return valueChanged;
}

bool imgui_acc_gdk::Checkbox(const char* label, bool* v)
{
    ImGuiStyle& style = ImGui::GetStyle();

    // Calculate the total size of the checkbox and label
    ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);
    float square_sz = ImGui::GetFrameHeight(); // Size of the checkbox square
    ImVec2 total_size;
    total_size.x = square_sz + style.ItemInnerSpacing.x + label_size.x;
    total_size.y = max(square_sz, label_size.y);

    // Enforce minimum size
    ImVec2 size_diff;
    size_diff.x = max(MINWIDGETSIZE.x - total_size.x, 0.0f);
    size_diff.y = max(MINWIDGETSIZE.y - total_size.y, 0.0f);

    // Push style vars if adjustments are needed as imgui checkbox doesn't have a size parameter
    if (size_diff.x > 0.0f || size_diff.y > 0.0f)
    {
        ImVec2 padding = { 0,0 };
        padding.x = style.FramePadding.x + size_diff.x / 2.0f;
        padding.y = style.FramePadding.y + size_diff.y / 2.0f;
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, padding);
    }

    bool pressed = ImGui::Checkbox(label, v);

    // Pop style vars if they were pushed
    if (size_diff.x > 0.0f || size_diff.y > 0.0f)
    {
        ImGui::PopStyleVar();
    }

    char buffer[256] = {};

    if (pressed)
    {
        if (*v)
            sprintf_s(buffer, 256, u8"%s %s\n", label, "checked.");
        else
            sprintf_s(buffer, 256, u8"%s %s\n", label, "unchecked.");
        
        m_narrator->AddPendingNarration(buffer);
    }
    return pressed;
}

bool imgui_acc_gdk::SliderInt(const char* label, int* v, int v_min, int v_max, const char*, ImGuiSliderFlags flags)
{
    // Direct input not supported. Support requires determining when ImGui::TempInputScalar is active in order to display the virtual keyboard.
    ImGui::BeginGroup();
    float slider_height = ImGui::GetFrameHeight();

    if (slider_height < MINWIDGETSIZE.y)
    {
        float padding_increase = (MINWIDGETSIZE.y - slider_height) / 2.0f;

        ImVec2 padding = ImGui::GetStyle().FramePadding;
        padding.y += padding_increase;
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, padding);
    }

    bool result = ImGui::SliderScalar(label, ImGuiDataType_S32, v, &v_min, &v_max, "", flags | ImGuiSliderFlags_NoInput);

    char buffer[MAXBUFFERSIZE] = {};
    if (ImGui::IsItemActive())
    {
        sprintf_s(buffer, MAXBUFFERSIZE, u8"Editing %s %d\n", label, *v);
        m_narrator->AddPendingNarration(buffer);
    }
    else if (ImGui::IsItemFocused())
    {
        sprintf_s(buffer, MAXBUFFERSIZE, u8"%s %d\n", label, *v);
        m_narrator->AddPendingNarration(buffer);
    }

    // Pop style var if it was pushed
    if (slider_height < MINWIDGETSIZE.y)
    {
        ImGui::PopStyleVar();
    }

    ImGui::SameLine();
    ImGui::Text(": %d", *v);
    ImGui::EndGroup();

    return result;
}

bool imgui_acc_gdk::WindowHeader(const char* text)
{
    // Create a unique ID for the selectable item based on the text
    ImGui::PushID(text);

    ImVec2 windowSize = ImGui::GetWindowSize();
    ImVec2 textSize = ImGui::CalcTextSize(text);

    // Calculate the position to start the text so it is centered
    float textPosX = (windowSize.x - textSize.x) * 0.5f;

    // Force selection to start on left side of window
    bool isSelected = ImGui::Selectable("");

    // Check if this widget is focused and trigger narration
    if (ImGui::IsItemFocused())
    {
        char buffer[256] = {};
        sprintf_s(buffer, 256, u8"%s window focused.\n", text);
        m_narrator->AddPendingNarration(buffer);
    }

    ImGui::SameLine();

    // Add text to the center of the line
    ImGui::SetCursorPosX(textPosX);
    ImGui::Text("%s", text);
    ImGui::Separator();
    ImGui::PopID();

    return isSelected;
}
#pragma endregion

#pragma region Accessibility Settings 
void imgui_acc_gdk::ApplyHighContrastDarkTheme()
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    //GPT attempt at Aquatic theme with slight modifications for accessiblity purposes
    colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);                // White text
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);        // Grey text (disabled)
    colors[ImGuiCol_WindowBg] = ImVec4(0.13f, 0.14f, 0.16f, 1.00f);            // Dark background
    colors[ImGuiCol_ChildBg] = ImVec4(0.13f, 0.14f, 0.16f, 1.00f);             // Dark background
    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);             // Very dark background
    colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);              // Grey border
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);        // No border shadow
    colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.231f, 0.313f, 1.00f);            // Dark blue-grey background
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.28f, 0.36f, 1.00f);        // Medium Blue on MOUSE hover #263B50 for header
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.32f, 0.40f, 0.47f, 1.00f);       // Slightly lighter blue-grey
    colors[ImGuiCol_TitleBg] = ImVec4(0.14f, 0.18f, 0.23f, 1.00f);             // Dark blue-grey background for title
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.20f, 0.25f, 1.00f);       // Slightly lighter for active title
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.14f, 0.18f, 0.23f, 1.00f);    // Same as window background for collapsed title
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.18f, 0.23f, 1.00f);           // Dark blue-grey background for menu bar
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.15f, 0.231f, 0.313f, 1.00f);        // Dark blue-grey background for scrollbar
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);       // Grey scrollbar grab
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);// Slightly lighter on hover
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.70f, 0.70f, 0.70f, 1.00f); // Lighter grey when active
    colors[ImGuiCol_CheckMark] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);            // White for check mark CHANGE THIS to WHITE
    colors[ImGuiCol_SliderGrab] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);         // White slider grab
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.56f, 0.89f, 0.94f, 1.00f);    // Lighter blue when active #8EE3F0
    colors[ImGuiCol_Button] = ImVec4(0.15f, 0.231f, 0.313f, 1.00f);             // Dark blue-grey background for button #202020
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.20f, 0.28f, 0.36f, 1.00f);      // Medium Blue on hover
    colors[ImGuiCol_ButtonActive] = ImVec4(0.32f, 0.40f, 0.47f, 1.00f);         // Lighter blue when active
    colors[ImGuiCol_Header] = ImVec4(0.15f, 0.231f, 0.313f, 1.00f);             // Dark blue-grey background for header
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.20f, 0.28f, 0.36f, 1.00f);        // Medium Blue on MOUSE hover #263B50 for header
    colors[ImGuiCol_HeaderActive] = ImVec4(0.32f, 0.40f, 0.47f, 1.00f);        // Lighter blue when active
    colors[ImGuiCol_Separator] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);           // Grey separator
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.57f, 0.57f, 0.64f, 1.00f);    // Lighter grey on hover
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.64f, 0.64f, 0.71f, 1.00f);     // Even lighter grey when active
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);          // Blue resize grip
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.56f, 0.89f, 0.94f, 1.00f);   // Lighter blue on hover
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.46f, 0.67f, 1.00f, 1.00f);    // Even lighter blue when active
    colors[ImGuiCol_Tab] = ImVec4(0.15f, 0.231f, 0.313f, 1.00f);                // Dark blue-grey background for tab
    colors[ImGuiCol_TabHovered] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);          // White on hover
    colors[ImGuiCol_TabActive] = ImVec4(0.56f, 0.89f, 0.94f, 1.00f);           // Lighter blue for active tab
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.15f, 0.231f, 0.313f, 1.00f);       // Dark blue-grey background for unfocused tab
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);  // Blue for active unfocused tab
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);           // Grey plot lines
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);    // Orange plot lines on hover
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);       // Grey plot histogram
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);// Orange plot histogram on hover
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.15f, 0.231f, 0.313f, 1.00f);      // Dark blue-grey background for table header       LEVEL 2
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);   // Grey for strong table border
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.57f, 0.57f, 0.64f, 1.00f);    // Lighter grey for light table border
    colors[ImGuiCol_TableRowBg] = ImVec4(0.13f, 0.14f, 0.16f, 1.00f);          // Dark background for table row background (even rows)
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.15f, 0.231f, 0.313f, 1.00f);      // Dark blue-grey background for table row background (odd rows)
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.15f, 0.231f, 0.313f, 1.00f);    // Medium Blue background for selected text #263B50
    colors[ImGuiCol_DragDropTarget] = ImVec4(0.56f, 0.89f, 0.94f, 1.00f);      // Lighter blue for drag drop target
    colors[ImGuiCol_NavHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);        // White for navigation highlight
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f); // White for windowing highlight with transparency
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.20f, 0.25f, 0.29f, 0.20f);   // Dark blue-grey background with transparency for dim background
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f, 0.25f, 0.29f, 0.35f);    // Dark blue-grey background with transparency for modal dim background
}

void imgui_acc_gdk::ApplyHighContrastLightTheme()
{
    //GPT attempt at Desert theme modified to match it better
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    colors[ImGuiCol_Text] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);                // Dark grey text
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);        // Grey text (disabled)
    colors[ImGuiCol_WindowBg] = ImVec4(0.95f, 0.95f, 0.90f, 1.00f);            // Light grey background
    colors[ImGuiCol_ChildBg] = ImVec4(0.95f, 0.95f, 0.90f, 1.00f);             // Light grey background
    colors[ImGuiCol_PopupBg] = ImVec4(0.80f, 0.80f, 0.75f, 1.00f);             // Light background
    colors[ImGuiCol_Border] = ImVec4(0.45f, 0.30f, 0.10f, 1.00f);              // Dark brown border
    colors[ImGuiCol_BorderShadow] = ImVec4(0.10f, 0.10f, 0.10f, 0.50f);        // Dark grey shadow with transparency
    colors[ImGuiCol_FrameBg] = ImVec4(0.80f, 0.80f, 0.75f, 1.00f);             // Light background
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.90f, 0.90f, 0.85f, 1.00f);      // Lighter shade on hover
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.80f, 0.80f, 0.75f, 1.00f);       // Same as FrameBg

    colors[ImGuiCol_TitleBg] = ImVec4(0.80f, 0.80f, 0.75f, 1.00f);             // Light background for title
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.70f, 0.70f, 0.65f, 1.00f);       // Slightly darker for active title
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.95f, 0.95f, 0.90f, 1.00f);    // Same as window background for collapsed title
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.95f, 0.95f, 0.90f, 1.00f);           // Light grey background for menu bar
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.80f, 0.80f, 0.75f, 1.00f);         // Light background for scrollbar
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);       // Grey scrollbar grab
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);// Slightly lighter on hover
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.4f, 0.30f, 0.10f, 1.00f); // Dark brown when active
    colors[ImGuiCol_CheckMark] = ImVec4(0.45f, 0.30f, 0.10f, 1.00f);           // Dark brown for check mark
    colors[ImGuiCol_SliderGrab] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);          // Grey slider grab
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.45f, 0.30f, 0.10f, 1.00f);    // Dark brown when active
    colors[ImGuiCol_Button] = ImVec4(0.80f, 0.80f, 0.75f, 1.00f);              // Light background for button
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.90f, 0.90f, 0.85f, 1.00f);       // Lighter shade on hover
    colors[ImGuiCol_ButtonActive] = ImVec4(0.45f, 0.30f, 0.10f, 1.00f);        // Dark brown when active
    colors[ImGuiCol_Header] = ImVec4(0.80f, 0.80f, 0.75f, 1.00f);              // Light background for header
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.90f, 0.90f, 0.85f, 1.00f);       // Lighter shade on hover
    colors[ImGuiCol_HeaderActive] = ImVec4(0.45f, 0.30f, 0.10f, 1.00f);        // Dark brown when active
    colors[ImGuiCol_Separator] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);           // Grey separator
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);    // Slightly lighter on hover
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.45f, 0.30f, 0.10f, 1.00f);     // Dark brown when active
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);          // Grey resize grip
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);   // Slightly lighter on hover
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.45f, 0.30f, 0.10f, 1.00f);    // Dark brown when active
    colors[ImGuiCol_Tab] = ImVec4(0.80f, 0.80f, 0.75f, 1.00f);                 // Light background for tab
    colors[ImGuiCol_TabHovered] = ImVec4(0.90f, 0.90f, 0.85f, 1.00f);          // Lighter shade on hover
    colors[ImGuiCol_TabActive] = ImVec4(0.70f, 0.70f, 0.65f, 1.00f);           // Slightly darker for active tab
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.80f, 0.80f, 0.75f, 1.00f);        // Light background for unfocused tab
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.70f, 0.70f, 0.65f, 1.00f);  // Slightly darker for active unfocused tab
    colors[ImGuiCol_PlotLines] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);           // Grey plot lines
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);    // Slightly lighter on hover
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);       // Grey plot histogram
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);// Slightly lighter on hover
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.80f, 0.80f, 0.75f, 1.00f);       // Light background for table header
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);   // Grey for strong table border
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);    // Slightly lighter for light table border

    colors[ImGuiCol_TableRowBg] = ImVec4(0.95f, 0.95f, 0.90f, 1.00f);         // Light grey for table row background (even rows)
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.90f, 0.90f, 0.85f, 1.00f);      // Slightly darker for table row background (odd rows)
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.80f, 0.80f, 0.75f, 1.00f);     // Light background for selected text
    colors[ImGuiCol_DragDropTarget] = ImVec4(0.70f, 0.70f, 0.65f, 1.00f);     // Slightly darker for drag drop target
    colors[ImGuiCol_NavHighlight] = ImVec4(0.45f, 0.30f, 0.10f, 1.00f);       // Dark brown for navigation highlight
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.70f, 0.70f, 0.65f, 1.00f); // Slightly darker for windowing highlight
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.75f, 0.50f);  // Light background with transparency for dim background
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.75f, 0.50f);   // Light background with transparency for modal dim background
}

void imgui_acc_gdk::AdjustForDisplayScaling()
{
    ImGui::GetStyle().ScaleAllSizes(m_scaleFactor);
    ImGuiIO& io = ImGui::GetIO();
    io.FontGlobalScale = m_scaleFactor;
}

void imgui_acc_gdk::DetectHighContrastTheme()
{
    XHighContrastMode highContrastMode;
    if (SUCCEEDED(XHighContrastGetMode(&highContrastMode)))
    {
        if (highContrastMode == XHighContrastMode::Dark)
        {
            ApplyHighContrastDarkTheme();
        }
        else if (highContrastMode == XHighContrastMode::Light)
        {
            ApplyHighContrastLightTheme();
        }
        else
        {
            ImGui::StyleColorsClassic();

            // Required for 4.5:1 contrast ratio
            ImGuiStyle& style = ImGui::GetStyle();
            ImVec4* colors = style.Colors;
            colors[ImGuiCol_HeaderHovered] = ImVec4(0.20f, 0.25f, 0.70f, 0.80f);
            colors[ImGuiCol_HeaderActive] = ImVec4(0.35f, 0.40f, 0.85f, 0.80f);
            colors[ImGuiCol_ButtonHovered] = ImVec4(0.20f, 0.25f, 0.70f, 0.80f);
            colors[ImGuiCol_ButtonActive] = ImVec4(0.35f, 0.40f, 0.85f, 0.80f);
        }
    }
}

void imgui_acc_gdk::EnableNarration()
{
    m_narrator->EnableNarration();
}

void imgui_acc_gdk::DisableNarration()
{
    m_narrator->DisableNarration();
}
#pragma endregion

#pragma region Virtual Keyboard
VirtualKeyboardManager::VirtualKeyboardManager()
{
    keyboardHandle = nullptr;
}

VirtualKeyboardManager::~VirtualKeyboardManager()
{
    if (keyboardHandle != nullptr)
    {
        XGameUiTextEntryClose(keyboardHandle);
    }
}

bool VirtualKeyboardManager::IsKeyboardActive()
{
    return keyboardHandle != nullptr;
}

void VirtualKeyboardManager::ActivateKeyboard(const char* buffer)
{
    XGameUiTextEntryOptions options;
    options.inputScope = XGameUiTextEntryInputScope::Default;
    options.positionHint = XGameUiTextEntryPositionHint::Bottom;
    options.visibilityFlags = XGameUiTextEntryVisibilityFlags::Default;

    // Get the total characters in the utf8 string
    int i = 0;
    // int length = 0;
    while (buffer[i])
    {
        // Future. Fix ImGui inputtext bugs before enabling UTF8 support from gdk side
        //if ((buffer[i] & 0xc0) != 0x80)
        //    length++;

        i++;
    }

    // Buffer taken from imgui inputtext is copied over to the gdk keyboard buffer
    // and not modified in any way.
    // Imgui manages the buffer lifetime
    XGameUiTextEntryOpen(&options,
        MAXBUFFERSIZE,
        buffer,
        i,
        &keyboardHandle);
}

void VirtualKeyboardManager::DeactivateKeyboard()
{
    XGameUiTextEntryClose(keyboardHandle);
    keyboardHandle = nullptr;
}
#pragma endregion
