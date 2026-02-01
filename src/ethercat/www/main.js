
let STORAGE = new Map();
let UPDATE = null;

function create_gui_table_control(guid, desc)
{
    // Возвращаем строку для заголовка
    function get_item_title(item)
    {
        // Если явно задана
        if ("title" in item)
        {
            // Возвращаем ее
            if (item.title.length)
                return item.title;

            // но если пустая то возвращаем значение bit
            if (item?.bit)
                return item.bit.toString();

            // или value
            if (item?.value)
                return item.value.toString();

            return null;
        }

        // Если не задана то возвращаем имя переменной, к которой привязан элемент
        return (("value-src" in item) ? item["value-src"] :
                (("value-dst" in item) ? item["value-dst"] : null));
    }

    function is_item_read_only(item)
    {
        if ("value-src" in item)
            return true;

        if ("value-dst" in item)
            return false;

        return null;
    }

    function get_value_name(item)
    {
        if ("value-src" in item)
            return item["value-src"];

        if ("value-dst" in item)
            return item["value-dst"];

        return null;
    }

    function add_common_header(thead, desc)
    {
        const head_text = get_item_title(desc);

        if (head_text === null)
            return;

        let cell = thead.insertRow().insertCell();
        cell.innerText = head_text ? head_text : "- - -";
        cell.setAttribute("colspan", desc.body.length);
    }

    function add_body_header(thead, desc)
    {
        let no_head_text_count = 0;

        let row = thead.insertRow();
        for (let item of desc.body)
        {
            let cell = row.insertCell();
            const head_text = get_item_title(item);
            cell.innerText = head_text ? head_text : "- - -";
            no_head_text_count += head_text ? 0 : 1;
        }

        // Если ни у одного элемента не задан вывод заголовка, удаляем строчку
        if (no_head_text_count == desc.body.length)
            thead.deleteRow(thead.rows.length - 1);
    }

    function add_body(tbody, desc)
    {
        const read_only = is_item_read_only(desc);

        let row = tbody.insertRow();
        {
            for (let field of desc.body)
            {
                let cell = row.insertCell();
                cell.innerText = field.label ? field.label : "- - -";

                let value_name = get_value_name(desc);
                if (value_name === null)
                    value_name = get_value_name(field);

                if (value_name !== null)
                {
                    let rec = UPDATE.get(value_name);
                    if (rec === undefined)
                    {
                        rec = new Array();
                        UPDATE.set(value_name, rec);
                    }

                    // Добавляем обработчик изменения значения для ячейки
                    if ("bit" in field)
                    {
                        rec.push((value) =>
                        {
                            const state = (parseInt(value) & (1 << field.bit)) != 0;
                            cell.className = state ? "state-on" : "state-off";

                            if ("userset" in cell)
                                cell.userset.value = state;
                        });
                    }
                    else if ("value" in field)
                    {
                        rec.push((value) => {
                            cell.className = (value === field.value.toString()) ? "state-on" : "state-off";
                        });
                    }
                    else
                    {
                        rec.push((value) => {
                            cell.innerText = value;
                        });
                    }
                }

                const ro = (read_only !== null) ? read_only : is_item_read_only(field);
                if (ro === null || ro)
                    continue;

                // Меняем курсор мыши при наведении
                cell.style.cursor = "pointer";

                // Добавляем обработчик нажатия
                if ("bit" in field)
                {
                    // Меняем значение соответствующего бита родительской переменной
                    cell.userset = { value: false, bit: field.bit };
                    cell.onclick = async () =>
                    {
                        cell.userset.value = !cell.userset.value;
                        cell.className = cell.userset.value ? "state-on" : "state-off";
                        const ac = cell.userset.value ? "set" : "reset"
                        await fetch(`/bit-${ac}?guid=${guid}&key=${desc["value-dst"]}&value=${field.bit}`, { method: "GET" });
                    };
                }
                // Меняем значение родительской переменной
                else if ("value" in field)
                {
                    cell.userset = { value: field.value };
                    cell.onclick = async () =>
                    {
                        Array.from(row.cells).forEach((cell) => { cell.className = null; });
                        cell.className = "state-on";
                        await fetch(`/value-set?guid=${guid}&key=${desc["value-dst"]}&value=${field.value}`, { method: "GET" });
                    };
                }
                // Показываем диалог задания значения
                else
                {
                    cell.onclick = () =>
                    {
                        create_dialog(async (value) => {
                            await fetch(`/value-set?guid=${guid}&key=${field["value-dst"]}&value=${value}`, { method: "GET" });
                        });
                    };
                }

                // cell.onclick = function()
                // {
                //     if ("bit" in field)
                //     {
                //         this.dataset.value = !this.dataset.value;
                //         this.className = this.dataset.value ? "bit-on" : "bit-off";
                //     }
                //     // Меняем значение родительской переменной
                //     else if ("value" in field)
                //     {
                //         // this.classList.add("inproc");
                //     }
                //     else
                //     {
                //     }
                // };
            }
        }
    }

    let table = document.createElement("table");
    table.userset = {
        update: (key, value) => {
            console.log(key, ' = ', value);
        }
    };

    {
        // Создаем заголовок таблицы
        let thead = table.createTHead();
        add_common_header(thead, desc);
        add_body_header(thead, desc);

        // Создаем тело таблицы и заполняем его значениями
        let tbody = table.createTBody();
        add_body(tbody, desc);
    }

    return table;
}

function create_gui_button_control(guid, desc)
{
    let button = document.createElement("button");
    button.className = desc.class;
    button.classList.add("btn");
    button.userdata = { active: false };

    async function on_press()
    {
        button.userdata.active = true;
        await fetch(`/value-set?guid=${guid}&key=${desc["value"]}&value=${desc["on-press"]}`, { method: "GET" });
    }

    async function on_release()
    {
        if (!button.userdata.active)
            return;
        button.userdata.active = false;
        await fetch(`/value-set?guid=${guid}&key=${desc["value"]}&value=${desc["on-release"]}`, { method: "GET" });
    }

    button.onmousedown = on_press;
    button.onmouseup = on_release;
    button.onmouseleave = on_release;

    return button;
}

// Создаем соответствующие графические элементы
function create_gui_controls(board, guid, ctls)
{
    if ("table" in ctls)
    {
        for (const item of ctls.table)
        {
            let ctl = create_gui_table_control(guid, item);
            board.appendChild(ctl);
        }
    }

    if ("button" in ctls)
    {
        for (const item of ctls.button)
        {
            let ctl = create_gui_button_control(guid, item);
            board.appendChild(ctl);
        }
    }
}

