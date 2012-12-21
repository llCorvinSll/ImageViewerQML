import QtQuick 2.0
import tk.corvinus.FileTreeModel 1.0
import "components"

Rectangle {
    width: 360
    height: 360

    Text {
        text: folderModel.parentFolder;
    }

    ListView {
        x: 20;
        y: 20;
        width: 200; height: 400

        model: FileTreeModel {
            id: folderModel
            nameFilters: ["*"]
        }

        delegate: Component {
            id: fileDelegate

            Text {
                text: fileName
                function up() {
                    folderModel.setFolder(folderModel.parentFolder);
                }
                MouseArea {
                    anchors.fill: parent;
                    onClicked: parent.up();
                }
            }
        }
    }


}
