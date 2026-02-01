
// Создаем таблицу формата
// +---------------------------------------+
// |                 title                 |
// +-----------+-------------+-------------+
// |  [0].idx  |   [1].idx   |  [2].idx    |
// +-----------+-------------+-------------+
// |  [0].title |  [1].title  |  [2].title |
// +------------+-------------+------------+

// cells [ { idx: number, title: text }, ... ]

function create_bits_table(title, fields, handler) 
{
    let table = document.createElement("table");
    {
        let header = table.createTHead();
        {
            // Создаем заголовок таблицы
            let row = header.insertRow();
            {
                let cell = row.insertCell();
                {
                    cell.innerText = title;
                    cell.setAttribute("colspan", fields.length);
                }
            }

            // Создаем строку с индексами битов
            row = header.insertRow();
            {
                for (let field of fields)
                {
                    let cell = row.insertCell();
                    cell.innerText = field.idx.toString();
                }
            }
        }
        
        // Создаем тело таблицы
        // и заполняем его значениями
        let body = table.createTBody();
        {
            const read_only = ("read_only" in handler) ? handler.read_only() : true;

            let row0 = body.insertRow();
            let row1 = read_only ? null : body.insertRow();
            {
                for (let field of fields)
                {
                    let cell = row0.insertCell();
                    cell.innerText = field.title;

                    if (row1 == null) 
                        handler.append(cell, field.idx);
                    else
                        handler.append(cell, row1.insertCell(), field.idx);
                }
            }
        }
    }
    return table;
}

// Создаем таблицу формата
// +---------------------------------------+
// |                 title                 |
// +-----------+-------------+-------------+
// |  [0].key  |   [1].key   |  [2].key    |
// +-----------+-------------+-------------+
// |  [0].value |  [1].value  |  [2].value |
// +------------+-------------+------------+

// cells [ { key: string, [title: string], value: value }, ... ]

function create_value_table(title, fields, handler) 
{
    let table = document.createElement("table");
    {
        let header = table.createTHead();
        {
            // Создаем заголовок таблицы
            let row = header.insertRow();
            {
                let cell = row.insertCell();
                {
                    cell.innerText = title;
                    cell.setAttribute("colspan", fields.length);
                }
            }
            // Создаем строку с индексами битов
            row = header.insertRow();
            {
                for (let field of fields)
                {
                    let cell = row.insertCell();
                    cell.innerText = field.key;
                }
            }
        }

        // Создаем тело таблицы
        // и заполняем его значениями
        let body = table.createTBody();
        {
            let row = body.insertRow();
            {
                let has_titles = false;
                for (let field of fields)
                {
                    let cell = row.insertCell();
                    let has_title = ("title" in field);
                    cell.innerText = has_title ? field.title : "";
                    has_titles |= has_title;
                }
                // Если мы не добавили ни одного заголовка
                // удаляем эту строку таблицы
                if (!has_titles)
                    body.deleteRow(body.rows.length - 1);
            }
            row = body.insertRow();
            {
                for (let field of fields)
                {
                    let cell = row.insertCell();
                    cell.innerText = " - ";
                    handler.append(cell, field.key);
                }
            }
        }
    }
    return table;
}

function create_one_row_table(title, fields, handler) 
{
    let table = document.createElement("table");
    {
        let header = table.createTHead();
        {
            // Создаем заголовок таблицы
            let row = header.insertRow();
            {
                let cell = row.insertCell();
                {
                    cell.innerText = title;
                    cell.setAttribute("colspan", fields.length);
                }
            }
        }
        
        // Создаем тело таблицы
        // и заполняем его значениями
        let body = table.createTBody();
        {
            let row = body.insertRow();
            {
                for (let field of fields)
                {
                    let cell = row.insertCell();
                    cell.innerText = field?.title ? field.title : field.value;
                    handler.append(cell, field.value);
                }
            }
        }
    }
    return table;
}
