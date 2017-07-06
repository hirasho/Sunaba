#include "../SunabaEnvForC.h" /* 同じフォルダにSunabaEnvForC.hがあるなら、 ../ は削除。*/

void clear();

void SunabaEnv_main(){
   SunabaEnv_disableAutoSync();
   while (SunabaEnv_getMouseRightButton() == 0){
      int x, y;
      SunabaEnv_sync();
      x = SunabaEnv_getMouseX();
      y = SunabaEnv_getMouseY();
      if (SunabaEnv_getMouseLeftButton()){
         SunabaEnv_drawPoint(x, y, 999999);
      }else if (SunabaEnv_getKeySpace()){
         clear();
      }
   }
}

void clear(){
   int x, y;
   for (x = 0; x < 100; ++x){
      for( y = 0; y < 100; ++y){
         SunabaEnv_drawPoint(x, y, 0);
      }
   }
}
