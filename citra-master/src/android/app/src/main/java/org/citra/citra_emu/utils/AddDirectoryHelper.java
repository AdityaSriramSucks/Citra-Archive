package org.citra.citra_emu.utils;

import android.content.AsyncQueryHandler;
import android.content.ContentValues;
import android.content.Context;
import android.net.Uri;

import org.citra.citra_emu.model.GameDatabase;
import org.citra.citra_emu.model.GameProvider;

public class AddDirectoryHelper {
    private Context mContext;

    public AddDirectoryHelper(Context context) {
        this.mContext = context;
    }

    public void addDirectory(String dir, AddDirectoryListener addDirectoryListener) {
        AsyncQueryHandler handler = new AsyncQueryHandler(mContext.getContentResolver()) {
            @Override
            protected void onInsertComplete(int token, Object cookie, Uri uri) {
                addDirectoryListener.onDirectoryAdded();
            }
        };

        ContentValues file = new ContentValues();
        file.put(GameDatabase.KEY_FOLDER_PATH, dir);

        handler.startInsert(0,                // We don't need to identify this call to the handler
                null,                        // We don't need to pass additional data to the handler
                GameProvider.URI_FOLDER,    // Tell the GameProvider we are adding a folder
                file);
    }

    public interface AddDirectoryListener {
        void onDirectoryAdded();
    }
}
