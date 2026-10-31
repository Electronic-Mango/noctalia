#include "shell/settings/tray_icon_override_control.h"

#include "core/files/directory_scanner.h"
#include "i18n/i18n.h"
#include "shell/settings/path_browse.h"
#include "shell/tray/tray_icon_override.h"
#include "ui/builders.h"
#include "ui/dialogs/file_dialog.h"
#include "ui/dialogs/glyph_picker_dialog.h"
#include "ui/style.h"
#include "util/file_utils.h"

#include <filesystem>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

namespace settings {

  std::unique_ptr<Node> makeTrayIconOverrideControl(
      float scale, std::shared_ptr<std::string> value, std::function<void(const std::string&)> commit
  ) {
    const auto parsed = tray::parseIconOverride(*value);
    auto source = std::make_shared<std::size_t>(parsed ? parsed->source : 0);
    auto sourceChosen = std::make_shared<bool>(parsed.has_value());
    auto text = std::make_shared<std::string>(parsed ? std::string(parsed->value) : *value);
    const auto updateValue = [source, sourceChosen, text, value] {
      *value = *sourceChosen ? tray::encodeIconOverride(*source, *text) : *text;
    };
    const auto submit = [value, commit] {
      if (commit) {
        commit(*value);
      }
    };

    Input* input = nullptr;
    auto textControl = ui::input({
        .out = &input,
        .value = *text,
        .fontSize = Style::fontSizeBody * scale,
        .controlHeight = Style::controlHeight * scale,
        .horizontalPadding = Style::spaceSm * scale,
        .height = Style::controlHeight * scale,
        .flexGrow = 1.0F,
        .onChange = [text, updateValue](const std::string& next) {
          *text = next;
          updateValue();
        },
        .onSubmit = [submit](const std::string&) { submit(); },
        .submitOnFocusLoss = true,
    });
    input->setInvalid(!parsed.has_value());

    // The picker may complete after the row has been rebuilt.
    auto inputHandle = std::make_shared<Input*>(input);
    Button* picker = nullptr;
    auto pickerControl = ui::button({
        .out = &picker,
        .glyphSize = Style::fontSizeBody * scale,
        .variant = ButtonVariant::Ghost,
        .minWidth = Style::controlHeight * scale,
        .minHeight = Style::controlHeight * scale,
        .onClick = [source, text, value, submit, inputHandle]() {
          const auto pickedSource = *source;
          const auto apply = [pickedSource, text, value, submit,
                              weakInput = std::weak_ptr<Input*>(inputHandle)](const std::string& picked) {
            *text = picked;
            *value = tray::encodeIconOverride(pickedSource, picked);
            if (auto handle = weakInput.lock()) {
              (*handle)->setValue(picked);
              (*handle)->setInvalid(false);
            }
            submit();
          };
          if (tray::iconOverrideSources[pickedSource] == "file") {
            FileDialogOptions options;
            options.mode = FileDialogMode::Open;
            options.defaultViewMode = FileDialogViewMode::Grid;
            options.title = i18n::tr("settings.widgets.settings.custom-image.dialog-title");
            options.extensions = DirectoryScanner::imageExtensionFilter(true);
            options.startDirectory = "/usr/share/icons";
            applyPathDialogStartValue(options, FileUtils::expandUserPath(*text).string(), PathBrowseKind::File);
            (void)FileDialog::open(std::move(options), [apply](std::optional<std::filesystem::path> picked) {
              if (picked) {
                apply(picked->string());
              }
            });
          } else if (tray::iconOverrideSources[pickedSource] == "tabler") {
            GlyphPickerDialogOptions options;
            if (!text->empty()) {
              options.initialGlyph = *text;
            }
            (void)GlyphPickerDialog::open(std::move(options), [apply](std::optional<GlyphPickerResult> picked) {
              if (picked) {
                apply(picked->name);
              }
            });
          }
        },
    });
    const auto refresh = [source, input, picker] {
      const auto kind = tray::iconOverrideSources[*source];
      input->setPlaceholder(i18n::tr(
          kind == "file" ? "settings.widgets.map-placeholders.icon-path"
          : kind == "tabler" ? "settings.widgets.map-placeholders.glyph-name"
                            : "settings.widgets.map-placeholders.icon-name"
      ));
      picker->setVisible(kind != "desktop");
      picker->setParticipatesInLayout(kind != "desktop");
      picker->setGlyph(kind == "file" ? "photo" : "apps");
      picker->setTooltip(i18n::tr(
          kind == "file" ? "settings.widgets.map-placeholders.browse-icon"
                        : "settings.widgets.map-placeholders.pick-icon"
      ));
    };
    refresh();
    auto selector = ui::select({
        .options = std::vector<std::string>{
            i18n::tr("settings.widgets.icon-override.desktop"),
            i18n::tr("settings.widgets.icon-override.file"),
            i18n::tr("settings.widgets.icon-override.tabler"),
        },
        .selectedIndex = *source,
        .clearSelection = !parsed.has_value(),
        .fontSize = Style::fontSizeBody * scale,
        .controlHeight = Style::controlHeight * scale,
        .width = 140.0F * scale,
        .onSelectionChanged = [source, sourceChosen, input, updateValue, refresh, submit](
                                  std::size_t index, std::string_view
                              ) {
          *source = index;
          *sourceChosen = true;
          input->setInvalid(false);
          updateValue();
          refresh();
          submit();
        },
    });
    return ui::row(
        {.align = FlexAlign::Center, .gap = Style::spaceSm * scale, .flexGrow = 3.0F},
        std::move(selector), std::move(textControl), std::move(pickerControl)
    );
  }

} // namespace settings
