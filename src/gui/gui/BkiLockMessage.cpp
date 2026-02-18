#include "BkiLockMessage.h"

BkiLockMessage::BkiLockMessage(QWidget *parent)
    : MessageBox(parent, MessageBox::HeadError)
{
    set_source("БКИ");
    set_message("Устранить контакт с индуктором");
    set_button("Активировать защиту");
}

void BkiLockMessage::show()
{
    MessageBox::show([this](std::size_t) {
        on_button_click();
    });
}

void BkiLockMessage::on_button_click()
{
    // global::rpc().call("set", { "bki", "reset", {} })
    //     .done([this](nlohmann::json const&)
    //     {
    //         aem::log::info("BkiLockMessage: OK");
    //     })
    //     .error([this](std::string_view emsg)
    //     {
    //         aem::log::error("BkiLockMessage: {}", emsg);
    //     });
}
