#ifndef __FAR_WRAPPERS_STANDART_DIALOGS
#define __FAR_WRAPPERS_STANDART_DIALOGS

namespace FARWrappers
{

struct FLngColorDialog
{
	std::wstring title_;         ///< Message for dialog title (Default text: "Colors").
	std::wstring fore_;          ///< Message for foreground label (Default text: "Fore").
	std::wstring bk_;            ///< Message for background label (Default text: "Back").
	std::wstring set_;           ///< Message for set button (Default text: "Set").
	std::wstring cancel_;        ///< Message for cancel button (Default text: "Cancel").
	std::wstring text_;          ///< Message for sample text (Default text: "text text text").
};

int ShowColorColorDialog(int color, FLngColorDialog* p, const std::wstring &help);

} // namespace FARWrappers

#endif