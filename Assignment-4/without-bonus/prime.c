int _start(){
  int number =150;
  int prime=1;
  for(int i=3;i<number;i++){
    if(number%i==0){
      prime=0;
    }
  }
  return prime==1?number:-1;
}