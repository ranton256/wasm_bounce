console.log("loaded main.js");

function main() {
    console.log("enter main()");
    console.log("exit main()");
}

function ContentLoadedHandler(evt) {
    console.log("ContentLoadedHandler");
}

if (document.readyState === 'loading') {  // Loading hasn't finished yet
    document.addEventListener('DOMContentLoaded', (event) => {
        console.log('DOMContentLoaded\n');
        ContentLoadedHandler();
    });
} else {  // `DOMContentLoaded` has already fired
    console.log('DOMContentLoaded has already fired.\n');
    ContentLoadedHandler();
}




