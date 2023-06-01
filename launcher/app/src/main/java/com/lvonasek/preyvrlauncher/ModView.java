package com.lvonasek.preyvrlauncher;

import android.content.Context;
import android.content.res.TypedArray;
import android.util.AttributeSet;
import android.view.View;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

public class ModView extends LinearLayout {

    private OnClickListener mListener;

    private LinearLayout mLayout;
    private ImageView mIcon;
    private TextView mTitle;
    private TextView mDesc;
    private String mFile;
    private String mMap;

    public ModView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        initView();
        getAttributes(context, attrs, defStyle);
    }

    public ModView(Context context, AttributeSet attrs) {
        super(context, attrs);
        initView();
        getAttributes(context, attrs, 0);
    }

    public ModView(Context context) {
        super(context);
        initView();
    }

    public String getFilename() {
        return mFile;
    }

    public String getMapname() {
        return mMap;
    }

    private void initView() {
        View root = inflate(getContext(), R.layout.lv_mods_view, this);
        mTitle = root.findViewById(R.id.title);
        mDesc = root.findViewById(R.id.desc);
        mIcon = root.findViewById(R.id.icon);

        mLayout = root.findViewById(R.id.item_layout);
        setOnClickListener(mListener);
    }

    private void getAttributes(Context context, AttributeSet attrs, int defStyleAttr) {
        TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.ModsView, defStyleAttr, 0);

        mIcon.setImageDrawable(a.getDrawable(R.styleable.ModsView_icon));
        mTitle.setText(a.getString(R.styleable.ModsView_title));
        mDesc.setText(a.getString(R.styleable.ModsView_desc));
        mFile = a.getString(R.styleable.ModsView_file);
        mMap = a.getString(R.styleable.ModsView_map);

        a.recycle();
    }

    @Override
    public void setOnClickListener(OnClickListener listener) {
        mListener = listener;
        if (mLayout != null) {
            mLayout.setOnClickListener(mListener);
            mTitle.setOnClickListener(mListener);
            mDesc.setOnClickListener(mListener);
            mIcon.setOnClickListener(mListener);
        }
    }
}
