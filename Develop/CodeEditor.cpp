#include "CodeEditor.h"
#if __has_include("CodeEditor.g.cpp")
#include "CodeEditor.g.cpp"
#endif

using namespace winrt;
using namespace Shared;
using namespace std::string_literals;
using namespace Windows::Foundation;
using namespace Windows::System;
using namespace Windows::Storage;
using namespace Windows::UI::Core;
using namespace Windows::UI::Text;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::ApplicationModel::DataTransfer;

namespace winrt::Develop::implementation
{
	CodeEditor::CodeEditor(StorageFile const& file) : m_WorkingFile(file)
	{
		InitializeComponent();

	}

	fire_and_forget CodeEditor::LoadFileAsync()
	{
		co_await TextView().LoadFileAsync(m_WorkingFile);
		m_HotKeyToken = CoreApplication::GetCurrentView().CoreWindow().Dispatcher().AcceleratorKeyActivated({ this, &CodeEditor::KeyPressHandler });
		m_ShareRequestToken = DataTransferManager::GetForCurrentView().DataRequested({ this, &CodeEditor::Editor_ShareRequested });
	}

	bool CodeEditor::Saved() const
	{
		return m_Saved;
	}

	void CodeEditor::Saved(bool value)
	{
		if (m_Saved != value)
		{
			m_Saved = value;
			m_PropertyChanged(*this, PropertyChangedEventArgs(L"Saved"));
		}
	}

	StorageFile CodeEditor::WorkingFile()
	{
		return m_WorkingFile;
	}

	bool CodeEditor::StartClosing()
	{
		Bindings->StopTracking();
		return true;
	}

	IAsyncAction CodeEditor::CloseAsync()
	{
		if (!m_Saved) co_await SaveFile_Click(nullptr, nullptr);
		Close();
	}

	void CodeEditor::Close()
	{
		// Remove event handlers
		CoreApplication::GetCurrentView().CoreWindow().Dispatcher().AcceleratorKeyActivated(m_HotKeyToken);
		DataTransferManager::GetForCurrentView().DataRequested(m_ShareRequestToken);
	}

	void CodeEditor::KeyPressHandler(Windows::UI::Core::CoreDispatcher const&, AcceleratorKeyEventArgs const& e)
	{
		if (!AppSettings::DialogShown() && !m_Unloaded && e.EventType() == CoreAcceleratorKeyEventType::KeyDown &&
			(CoreApplication::GetCurrentView().CoreWindow().GetKeyState(VirtualKey::Control) & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down)
		{
			switch (e.VirtualKey())
			{
			case VirtualKey::Z:
				// Handle Ctrl + Z
				e.Handled(true);
				TextView().HistoryStack().Undo();
				break;
				
			case VirtualKey::Y:
				// Handle Ctrl + Y
				e.Handled(true);
				TextView().HistoryStack().Redo();
				break;

			case VirtualKey::S:
				// Handle Ctrl + S
				e.Handled(true);
				SaveFile_Click(nullptr, nullptr);
				break;

			default:
				// Don't handle the event
				e.Handled(false);
				break;
			}
		}
	}

	void CodeEditor::Editor_ShareRequested(DataTransferManager const&, DataRequestedEventArgs const& e)
	{
		auto deferral = e.Request().GetDeferral();
		if (TextView().IsSelectionValid())
		{
			auto text = TextView().SelectionText();
			TextView().IsRichText() ? e.Request().Data().SetRtf(text) : e.Request().Data().SetText(text);
		}
		else
		{
			e.Request().Data().SetStorageItems({ m_WorkingFile });
			e.Request().Data().SetText(TextView().Text());
		}

		deferral.Complete();
	}

	void CodeEditor::UserControl_Loaded(IInspectable const&, RoutedEventArgs const&)
	{
		m_Unloaded = false;
		if (!TextView().FileLoaded()) LoadFileAsync();
		else TextView().AttachEvents();
	}

	void CodeEditor::UserControl_Unloaded(IInspectable const&, RoutedEventArgs const&)
	{
		m_Unloaded = true;
		TextView().DetachEvents();
	}

	void CodeEditor::StandardCommand_Loaded(IInspectable const& sender, RoutedEventArgs const&)
	{
		auto target = sender.as<AppBarButton>();
		auto tag = unbox_value<hstring>(target.Tag());
		StandardUICommand command;
		if (tag == L"Undo") command.Kind(StandardUICommandKind::Undo);
		else if (tag == L"Redo") command.Kind(StandardUICommandKind::Redo);
		else if (tag == L"SelectAll") command.Kind(StandardUICommandKind::SelectAll);
		else if (tag == L"Save") command.Kind(StandardUICommandKind::Save);
		if (target.Label().empty())
		{
			IconSourceElement iconElement;
			iconElement.IconSource(command.IconSource());
			target.Label(command.Label());
			target.Icon(iconElement);
			target.AccessKey(command.AccessKey());
			ToolTipService::SetToolTip(target, box_value(command.Description()));
		}
	}

	IAsyncAction CodeEditor::SaveFile_Click(IInspectable const&, RoutedEventArgs const&)
	{
		if (TextView().FileLoaded() && !m_FileSaveLock)
		{
			m_FileSaveLock = true;
			auto lifetime = get_strong();
			if (!co_await TextView().WriteFileAsync(m_WorkingFile))
			{
				// TODO Work on recovery system
			}
			else Saved(true);
			m_FileSaveLock = false;
		}
	}

	event_token CodeEditor::PropertyChanged(PropertyChangedEventHandler const& handler) noexcept
	{
		return m_PropertyChanged.add(handler);
	}

	void CodeEditor::PropertyChanged(event_token token) noexcept
	{
		m_PropertyChanged.remove(token);
	}

	void CodeEditor::EditorCommand_Requested(IInspectable const& sender, RoutedEventArgs const&)
	{
		if (!TextView().FileLoaded()) return;
		auto target = sender.as<AppBarButton>();
		auto tag = unbox_value<hstring>(target.Tag());
		if (tag == L"Undo") TextView().HistoryStack().Undo();
		else if (tag == L"Redo") TextView().HistoryStack().Redo();
		else if (tag == L"SelectAll") TextView().SelectAll();
		else if (tag == L"ClearSelection") TextView().ClearSelection();
	}

	void CodeEditor::TextView_ContentChanged(bool isReset)
	{
		if (isReset || AppSettings::Preferences().AutoSave()) SaveFile_Click(nullptr, nullptr);
		else Saved(false);
	}

	void CodeEditor::Share_Click(IInspectable const&, RoutedEventArgs const&)
	{
		DataTransferManager::ShowShareUI();
	}

	CodeEditor::~CodeEditor()
	{
		// For tracking memory leaks
		OutputDebugString((L"Code editor for "s + m_WorkingFile.Path() + L" has been destroyed\r").data());
	}
}