package com.example.owner.ford;

import android.support.v7.app.ActionBarActivity;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.TextView;

import java.util.ArrayList;


public class MainActivity extends ActionBarActivity {

    public static final Integer[] images = { R.drawable.focus,
    R.drawable.fusion,
    R.drawable.mustang};


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        ArrayList<Car> cars = new ArrayList<Car>();
        Car car1 = new Car();
        car1.setModel("Focus");
        cars.add(car1);


        ListView listView = (ListView)findViewById(R.id.car_list);
        listView.setAdapter(new ModelAdapter(cars));
    }

    private class ModelAdapter extends ArrayAdapter<Car> {
        public ModelAdapter(ArrayList<Car> model) {
            super(MainActivity.this, R.layout.car_list_row, R.id.car_row_name, model);
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            convertView = super.getView(position, convertView, parent);

            Car car = getItem(position);

            TextView nameTextView = (TextView)convertView.findViewById(R.id.car_row_name);
            nameTextView.setText(car.getModel());
            return convertView;
        }
    }
    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();

        //noinspection SimplifiableIfStatement
        if (id == R.id.action_settings) {
            return true;
        }

        return super.onOptionsItemSelected(item);
    }
}
