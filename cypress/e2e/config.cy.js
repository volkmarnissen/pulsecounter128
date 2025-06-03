describe('template spec', () => {
  it('passes', () => {
    cy.intercept("GET", "**/config", {
    fixture: "config.json",
  });
    cy.visit('./lib/common/html/index.html')
  })
})