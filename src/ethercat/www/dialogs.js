
function create_dialog(callback)
{
    let overlay = document.createElement("div");
    overlay.className = "modal-overlay";
    {
        let modal = document.createElement("div");
        modal.className = "modal";
        {
            let header = document.createElement("div");
            header.className = "modal-header";
            modal.appendChild(header);

            let body = document.createElement("div");
            body.className = "modal-body";
            {
                let input = document.createElement("input");
                body.user_input = input;
                input.type = "number";
                body.appendChild(input);
            }
            modal.appendChild(body);

            let footer = document.createElement("div");
            footer.className = "modal-footer";
            {
                let button = document.createElement("button");
                button.innerText = "ОК";
                button.onclick = () => {
                    overlay.remove();
                    callback(body.user_input.valueAsNumber);
                };
                footer.appendChild(button);

                button = document.createElement("button");
                button.innerText = "Отмена";
                button.onclick = () => {
                    overlay.remove();
                };
                footer.appendChild(button);
            }
            modal.appendChild(footer);
        }
        overlay.appendChild(modal);
    }
    document.body.appendChild(overlay);
}
