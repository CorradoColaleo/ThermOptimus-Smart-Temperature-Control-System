#include "stm32f3xx.h"                  // Device header

//Dichiarazione funzioni
void abilitazione_periferiche();
void ADC_setup();
void TIM3_setup();                               
void TIM3_reset();
void TIM2_setup();
void TIM2_reset();
void TIM6_reset();
void TIM7_reset();
void PWM_reset();
void PWM_setup();
void HANDLER_setup();
void TIM6_DAC_IRQHandler();
void TIM7_IRQHandler();
void controllo_temperatura();

//Dichiarazioni variabili globali
float Tmin=15.0;
float Tmax=18.0;
float temperatura=0;
float Tbasso=24;
float Tmedio=25;
float tensione=0;
float numero_di_controlli=0;

//Variabili per il setup dei TIMER usati per gestire la PWM
	unsigned int Ton_PSC=0;
	unsigned int Ton_ARR=0;
	unsigned int T_PSC=0;
	unsigned int T_ARR=320;
	
int main ()
{
	
	//Abilito le periferiche
	abilitazione_periferiche();
	
	//Setup del timer
	TIM3_setup();
	TIM2_setup();
	
	//Setup dell'ADC
	ADC_setup();
	
	//Setup Handler
	HANDLER_setup();
	
	//Setup della PWM
	PWM_setup();
	
	//Abilito il TIM2
	TIM2->CR1|= TIM_CR1_CEN;
	
	
	while (1){
		
	if ((TIM2->SR & TIM_SR_UIF)==TIM_SR_UIF)
		
	{
		
	//Attivo la conversione
	ADC1->CR |= ADC_CR_ADSTART;	
	
	//Attendo il termine della conversione
	while ((ADC1->ISR & ADC_ISR_EOC) != ADC_ISR_EOC);
		
	//Leggo il risultato
	tensione = (float) (ADC1->DR * 3.0/4096.0);
	temperatura = (float) (25.0 + ((1.43-tensione)/0.0043));
	numero_di_controlli++;
		
	//reset della pwm
	PWM_reset();
	
	//Gestione led / ventola
	controllo_temperatura();
	
	//reset del TIM2
	TIM2_reset();
	
	}
}
	
}

void abilitazione_periferiche()
{

	//Abilito il clock all'ADC e alla GPIOC
	RCC->AHBENR |= RCC_AHBENR_ADC12EN | RCC_AHBENR_GPIOCEN;
	
	//Imposto le seguenti linee della GPIOC come output 
	GPIOC->MODER |= GPIO_MODER_MODER1_0| GPIO_MODER_MODER12_0 | GPIO_MODER_MODER11_0 | GPIO_MODER_MODER9_0;
	
	//Abilito il clock ai	timer TIM2, TIM3, TIM6, TIM7
	RCC->APB1ENR |= RCC_APB1ENR_TIM7EN |RCC_APB1ENR_TIM6EN | RCC_APB1ENR_TIM3EN | RCC_APB1ENR_TIM2EN;

}
//
//
//
//
void ADC_setup()
{
	//Abilito il regolatore di tensione interno dell'ADC
	ADC1->CR |= ADC_CR_ADVREGEN_1; //mi assicuro che il valore iniziale sia 10
	ADC1->CR &= (~ADC_CR_ADVREGEN); //10->00 
  ADC1->CR |= ADC_CR_ADVREGEN_0; //00->01
	
	/*Il software deve attendere il tempo di avvio del regolatore di tensione ADC (TADCVREG_STUP) 
	prima di avviare una calibrazione o abilitare l'ADC. TADCVREG_STUP è pari a 10 µs nel caso peggiore*/
	
	TIM3->CR1 |= TIM_CR1_CEN; //abilito il timer
	while ((TIM3->SR & TIM_SR_UIF) != TIM_SR_UIF); //attendo 10 microsecondi
	TIM3_reset(); //reset del timer
	
	//Imposto l'ADC Clock Mode: Sfrutto il clock del bus AHB (diviso per 1)
	ADC1_2_COMMON->CCR |= ADC12_CCR_CKMODE_0; //uso il clock del bus (diviso per 1)
	
	/*Impongo ADCALDIF=0 prima di lanciare la calibrazione che verrà applicata per le conversioni 
	di input a terminazione singola.*/
	ADC1->CR &= (~ADC_CR_ADCALDIF);
	
	//Avvio la procedura di calibrazione (ADCAL=1)
	ADC1->CR |= ADC_CR_ADCAL;
	
	//Aspetto il termine della calibrazione (ADCAL=0)
	while ((ADC1->CR & ADC_CR_ADCAL) == ADC_CR_ADCAL);
	
	//Alzo il bit TSEN per abilitare il sensore di temperatura
	ADC1_2_COMMON->CCR |= ADC12_CCR_TSEN;
	
	//Abilito l'ADC (ADEN=1)
	ADC1->CR |= ADC_CR_ADEN;
	
	//Aspetto che sia pronto (ADRDY=1)
	while ((ADC1->ISR & ADC_ISR_ADRD)!=ADC_ISR_ADRD);
	
	//Imposto la "single conversion mode" (CONT=0)
	ADC1->CFGR &= (~ADC_CFGR_CONT); //cont=0 -> singola conversione
	
	//Imposto la "regular channel sequence lenght"
	ADC1->SQR1 &= (~ADC_SQR1_L); //l=0 
	
	//Seleziono il canale
	ADC1->SQR1 |=  ADC_SQR1_SQ1_4; //abilito il canale 16
	
	/*Prima di iniziare una conversione, l'ADC deve stabilire una connessione diretta tra la sorgente di tensione 
	sotto misurazione e il condensatore di campionamento incorporato dell'ADC. Questo tempo di campionamento deve essere 
	sufficiente affinché la sorgente di tensione di ingresso carichi il condensatore incorporato al livello di 
	tensione di ingresso. Di seguito, seleziono il Sampling Time*/
	
	ADC1->SMPR2 |= ADC_SMPR2_SMP16; //111 = 601.5 colpi di clock	
	
	//Imposto come risoluzione: 12 bit
	ADC1->CFGR &= (~ADC_CFGR_RES); 
}

//
//
//
//
void controllo_temperatura()
{
	//Per ragioni di sicurezza, spengo tutti i led esterni.
	GPIOC->ODR &= (0xFFFFE5FF);
	
	if (temperatura>=Tmax)
	{
		GPIOC->ODR |= GPIO_ODR_12; //temperatura troppo calda, accendi led rosso.
		
		//frequenza scelta per la PWM -> 25 kHz (periodo di 40 microsecondi)
		
		if (temperatura<Tbasso)
		{
			//Imposto il duty cycle a 10%
			Ton_ARR=32;
		}
		
		else if ((temperatura >= Tbasso ) && (temperatura<=Tmedio))
		{
			//Imposto il duty cycle a 40%
			Ton_ARR=128;
		}			
		
		else if (temperatura>Tmedio)
		{
			//Imposto il duty cycle a 80%
			Ton_ARR=256;
		}
		
		//Configuro correttamente il TIM7->ARR
		TIM7->ARR=Ton_ARR;
		
		//Simulo il primo ciclo via software, in modo da non perdere neanche i primi 40 micro secondi
		TIM6->CR1 |= TIM_CR1_CEN;
		TIM7->CR1 |= TIM_CR1_CEN;
		GPIOC->ODR |= GPIO_ODR_1;
	}
	
	else if ((temperatura>Tmin) && (temperatura<Tmax))
	{
		GPIOC->ODR |= GPIO_ODR_9; //temperatura ottimale, accendi led verde
	}
	
	else if (temperatura<=Tmin)
	{
		GPIOC->ODR |= GPIO_ODR_11; //temperatura troppo fredda, accendi led blu
	
	}
	
}

//
//
//
//
void TIM3_setup()
{
	TIM3->CNT=0;
	TIM3->PSC=0;
	TIM3->ARR=80;
}
//
//
//
//
void TIM3_reset()
{
	TIM3->SR &= (~TIM_CR1_CEN); //spengo il timer
	TIM3->SR &= (~TIM_SR_UIF); //pulisco uif
	TIM3->CNT=0;
}
//
//
//
//
void TIM2_setup()
{
	TIM2->ARR=40000000;
	TIM2->PSC=0;
	TIM2->CNT=0;
}
//
//
//
//
void TIM2_reset()
{
	TIM2->SR &= (~TIM_SR_UIF); //pulisco uif
}

//
//
//
//
void TIM7_reset()
{
	TIM7->CR1 &= ~TIM_CR1_CEN;
	TIM7->SR &= ~TIM_SR_UIF;
	TIM7->CNT=0;
}
//
//
//
//
void TIM6_reset()
{
	TIM6->CR1 &= ~TIM_CR1_CEN;
	TIM6->SR &= ~TIM_SR_UIF;
	TIM6->CNT=0;
}
//
//
//
//
void PWM_setup()
{
	TIM6->ARR=T_ARR;
	TIM6->PSC = T_PSC;

	TIM7->PSC=Ton_PSC;
	TIM7->ARR=Ton_ARR;
}
//
//
//
//
void TIM6_DAC_IRQHandler()
{
	//quando TIM6->uif si alza
	TIM6->SR &=(~TIM_SR_UIF);
	GPIOC->ODR |= GPIO_ODR_1;
	TIM7->CR1 |= TIM_CR1_CEN;


}
//
//
//
//
void TIM7_IRQHandler()
{
	//quando TIM7->UIF si alza
	TIM7->CR1 &= (~TIM_CR1_CEN);
	TIM7->SR &= (~TIM_SR_UIF);
	GPIOC->ODR &= (~GPIO_ODR_1);	
	TIM7->CNT=0;
}
//
//
//
//
void HANDLER_setup()
{	
	TIM6->DIER |= TIM_DIER_UIE;
	TIM7->DIER |= TIM_DIER_UIE;
	NVIC->ISER[1] |= (1<<22) | (1<<23);
}
//
//
//
//
void PWM_reset(){
	TIM6_reset();
  TIM7_reset();
	GPIOC->ODR &= (~GPIO_ODR_1);	
}