#ifndef CH_DIALOGS_H
#define CH_DIALOGS_H

#include "engine/common/base.h"
#include <filesystem>
#include <string>
#include <vector>
#include <optional>

namespace Chained
{
	struct CH_API DialogFilter
	{
		std::string Name;
		std::string Spec;
	};

	enum class CH_API MessageBoxChoice
	{
		Ok,
		OkCancel,
		YesNo,
		YesNoCancel,
	};

	enum class CH_API MessageBoxIcon
	{
		Info,
		Warning,
		Error,
		Question,
	};

	enum class CH_API MessageBoxResult
	{
		Ok,
		Cancel,
		Yes,
		No,
	};

	class CH_API Dialogs
	{
	public:
		// Opens a file dialog and returns the selected path.
		static std::optional<std::filesystem::path> OpenFile(const std::vector<DialogFilter>& filters = {});

		// Opens a save file dialog and returns the selected path.
		static std::optional<std::filesystem::path> SaveFile(const std::vector<DialogFilter>& filters = {});

		// Opens a folder picker dialog and returns the selected path.
		static std::optional<std::filesystem::path> PickFolder();

		// Shows a modal message box and returns the button the user clicked.
		static MessageBoxResult ShowMessage(const std::string& title, const std::string& text,
											MessageBoxChoice choice = MessageBoxChoice::Ok,
											MessageBoxIcon icon = MessageBoxIcon::Info);

		// Convenience wrapper: shows a blocking error message box (OK button, error icon).
		static void ShowError(const std::string& title, const std::string& text)
		{
			ShowMessage(title, text, MessageBoxChoice::Ok, MessageBoxIcon::Error);
		}

		// Shows a non-blocking desktop notification.
		static void Notify(const std::string& title, const std::string& message,
						   MessageBoxIcon icon = MessageBoxIcon::Info);
	};
} // namespace Chained

#endif // CH_DIALOGS_H
