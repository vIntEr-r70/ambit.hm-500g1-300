
// Обработчик, обновляющий свои объекты по значению битовой маски
function create_bit_mask_view_handler()
{
    class handler
    {
        #content = new Array();

        // Добавляем новый объект
        append(dom_object, idx) {
            this.#content.push({ idx: idx, obj: dom_object });
        }

        // Обновляем состояние всех объектов
        update(bit_mask)
        {
            for (let item of this.#content)
            {
                const flag = ((bit_mask >> item.idx) & 0x1) !== 0;
                item.obj.style.backgroundColor = flag ? "#1ce83a" : null;
            }
        }
    };
    return new handler();
}

function create_bit_mask_set_handler()
{
    class handler
    {
        #content = new Array();

        // Добавляем новый объект
        append(dom_object0, dom_object1, idx)
        {
            dom_object1.style.cursor = "pointer";
            dom_object1.className = "BIT-OFF";
            const item_idx = this.#content.length;
            dom_object1.onclick = async () =>
            {
                let item = this.#content[item_idx];
                item.set = item.set ? false : true;
                this.#update_cell(item);
            };
            this.#content.push({ idx: idx, obj0: dom_object0, obj1: dom_object1, set: false, value: false });
        }

        #update_cell(item)
        {
            if (item.value != item.set)
                item.obj0.className = "blink";
            else
            {
                item.obj0.className = null;
                item.obj0.style.backgroundColor = item.value ? "#1ce83a" : null;
            }

		    item.obj1.className = item.set ? "BIT-ON" : "BIT-OFF";
        }

        // Обновляем состояние всех объектов
        update(bit_mask)
        {
            for (let item of this.#content)
            {
                item.value = ((bit_mask >> item.idx) & 0x1) !== 0;
                this.#update_cell(item);
            }
        }

        read_only() { return false; }
    };
    return new handler();
}

function create_value_view_handler()
{
    class handler
    {
        #content = new Map();

        // Добавляем новый объект
        append(dom_object, key)
        {
            this.#content.set(key, dom_object);
        }

        // Обновляем состояние всех объектов
        update(key, value)
        {
            let obj = this.#content.get(key) ?? null;
            if (obj != null) obj.innerText = value.toString();
        }
    };
    return new handler();
}

function create_value_set_handler()
{
    class handler
    {
        #content = new Map();

        // Добавляем новый объект
        append(dom_object, key)
        {
            dom_object.style.cursor = "pointer";
            dom_object.onclick = () =>
            {
                create_dialog((value) => {
                    console.log("SET ", value);
                });
            };
            this.#content.set(key, dom_object);
        }

        // Обновляем состояние всех объектов
        // update(key, value)
        // {
        //     let obj = this.#content.get(key) ?? null;
        //     if (obj != null) obj.innerText = value.toString();
        // }
    };
    return new handler();
}

function create_select_value_modify_handler()
{
    class handler
    {
        #content = new Map();

        // Добавляем новый объект
        append(dom_object, key)
        {
            this.#content.set(key, dom_object);

            dom_object.style.cursor = "pointer";
            dom_object.onclick = () =>
            {
                this.#content.set(key, dom_object);
            };
        }

        // Обновляем состояние всех объектов
        // update(key, value)
        // {
        //     let obj = this.#content.get(key) ?? null;
        //     if (obj != null) obj.innerText = value.toString();
        // }
    };
    return new handler();
}
