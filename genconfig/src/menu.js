const blessed = require('blessed');
const createHelpBox = require('./help-box');

const buttonStyle = () => ({
    bg: 'blue',
    focus: {
        bg: 'red'
    },
    hover: {
        bg: 'red'
    }
});

function getName(schema) {
    return schema.title;
}

function getMenuKeys(schema) {
    const propKeys = Object.keys(schema.properties);

    return propKeys.map((k) => schema.properties[k].title ? k : null).filter((k) => k);
}

function schema2ListItems(schema) {
    const propKeys = getMenuKeys(schema);

    return propKeys.map((k) => schema.properties[k].title).filter((title) => title);
}

function listIndex2subSchema(list, schema) {
    const index = list.selected;
    const propKeys = getMenuKeys(schema);

    return schema.properties[propKeys[index]];
}

module.exports = function createMenu(screen, schema, config) {
    const box = blessed.box({
        label: getName(schema),
        top: 'center',
        left: 'center',
        width: '50%',
        height: '70%',
        draggable: true,
        border: {
            type: 'line'
        },
        style: {
            fg: 'white',
            bg: 'magenta',
            border: {
                fg: 'white'
            }
        }
    });

    const form = blessed.form({
        parent: box,
        //keys: true,
        left: 0,
        right: 0,
        top: 0,
        bottom: 0,
        bg: 'green'
    });

    const helpBox = createHelpBox(form, schema);

    const list = blessed.list({
        parent: form,
        keys: true,
        vi: true,
        mouse: true,
        top: 5,
        bottom: 3,
        scrollbar: {
            ch: ' ',
            track: {
                bg: 'cyan'
            },
            style: {
                inverse: true
            }
        },
        style: {
            selected: {
                bg: 'blue',
                bold: true
            }
        }
    });
    const listItems = schema2ListItems(schema);
    list.setItems(listItems);

    const reset = blessed.button({
        parent: form,
        mouse: true,
        keys: true,
        shrink: true,
        padding: {
            left: 1,
            right: 1
        },
        right: 15,
        bottom: 1,
        name: 'reset',
        content: 'reset',
        style: buttonStyle()
    });

    const cancel = blessed.button({
        parent: form,
        mouse: true,
        keys: true,
        shrink: true,
        padding: {
            left: 1,
            right: 1
        },
        right: 6,
        bottom: 1,
        name: 'cancel',
        content: 'cancel',
        style: buttonStyle()
    });

    const submit = blessed.button({
        parent: form,
        mouse: true,
        keys: true,
        shrink: true,
        padding: {
            left: 1,
            right: 1
        },
        right: 1,
        bottom: 1,
        name: 'ok',
        content: 'ok',
        style: buttonStyle()
    });

    const txt = blessed.text({
        parent: form,
        left: 1,
        bottom: 1,
        style: { bg: 'green' }
    });

    screen.key('tab', function(ch, key) {
        form.focusNext();
    });
    screen.key('S-tab', function(ch, key) {
        form.focusPrevious();
    });

    list.key('enter', () => {
        const subSchema = listIndex2subSchema(list, schema);
        // TODO pass config nicely??
        if (subSchema.metaType === 'menu') {
            createMenu(screen, subSchema, config);
            screen.render();
        }
    });

    reset.on('press', function() {
        form.reset();
    });

    cancel.on('press', function() {
        form.cancel();
    });

    submit.on('press', function() {
        form.submit();
    });

    form.on('reset', function(data) {
        // TODO
        txt.setContent('Reset.');
        screen.render();
    });

    form.on('cancel', function(data) {
        // TODO Cancel.
        txt.setContent('Canceled.');
        box.destroy();
        screen.render();
    });

    form.on('submit', function(data) {
        txt.setContent('OK.');
        screen.render();
    });

    screen.append(box);

    return box;
}
