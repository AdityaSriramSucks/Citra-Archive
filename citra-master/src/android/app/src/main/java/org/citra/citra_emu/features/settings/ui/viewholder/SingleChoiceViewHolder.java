package org.citra.citra_emu.features.settings.ui.viewholder;

import android.content.res.Resources;
import android.view.View;
import android.widget.TextView;

import org.citra.citra_emu.R;
import org.citra.citra_emu.features.settings.model.view.PremiumSingleChoiceSetting;
import org.citra.citra_emu.features.settings.model.view.SettingsItem;
import org.citra.citra_emu.features.settings.model.view.SingleChoiceSetting;
import org.citra.citra_emu.features.settings.model.view.StringSingleChoiceSetting;
import org.citra.citra_emu.features.settings.ui.SettingsAdapter;

public final class SingleChoiceViewHolder extends SettingViewHolder {
    private SettingsItem mItem;

    private TextView mTextSettingName;
    private TextView mTextSettingDescription;

    public SingleChoiceViewHolder(View itemView, SettingsAdapter adapter) {
        super(itemView, adapter);
    }

    @Override
    protected void findViews(View root) {
        mTextSettingName = root.findViewById(R.id.text_setting_name);
        mTextSettingDescription = root.findViewById(R.id.text_setting_description);
    }

    @Override
    public void bind(SettingsItem item) {
        mItem = item;

        mTextSettingName.setText(item.getNameId());
        mTextSettingDescription.setVisibility(View.VISIBLE);
        if (item.getDescriptionId() > 0) {
            mTextSettingDescription.setText(item.getDescriptionId());
        } else if (item instanceof SingleChoiceSetting) {
            SingleChoiceSetting setting = (SingleChoiceSetting) item;
            int selected = setting.getSelectedValue();
            Resources resMgr = mTextSettingDescription.getContext().getResources();
            String[] choices = resMgr.getStringArray(setting.getChoicesId());
            int[] values = resMgr.getIntArray(setting.getValuesId());
            for (int i = 0; i < values.length; ++i) {
                if (values[i] == selected) {
                    mTextSettingDescription.setText(choices[i]);
                }
            }
        } else if (item instanceof PremiumSingleChoiceSetting) {
            PremiumSingleChoiceSetting setting = (PremiumSingleChoiceSetting) item;
            int selected = setting.getSelectedValue();
            Resources resMgr = mTextSettingDescription.getContext().getResources();
            String[] choices = resMgr.getStringArray(setting.getChoicesId());
            int[] values = resMgr.getIntArray(setting.getValuesId());
            for (int i = 0; i < values.length; ++i) {
                if (values[i] == selected) {
                    mTextSettingDescription.setText(choices[i]);
                }
            }
        } else {
            mTextSettingDescription.setVisibility(View.GONE);
        }
    }

    @Override
    public void onClick(View clicked) {
        int position = getAdapterPosition();
        if (mItem instanceof SingleChoiceSetting) {
            getAdapter().onSingleChoiceClick((SingleChoiceSetting) mItem, position);
        } else if (mItem instanceof PremiumSingleChoiceSetting) {
            getAdapter().onSingleChoiceClick((PremiumSingleChoiceSetting) mItem, position);
        } else if (mItem instanceof StringSingleChoiceSetting) {
            getAdapter().onStringSingleChoiceClick((StringSingleChoiceSetting) mItem, position);
        }
    }
}
